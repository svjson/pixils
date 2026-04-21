
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/style_namespace.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/render.h>

#include <SDL2/SDL_render.h>
#include <lisple/context.h>
#include <lisple/host.h>
#include <lisple/host/accessor.h>
#include <lisple/host/object.h>
#include <lisple/host/schema.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>
#include <unordered_map>

namespace
{
  /**
   * Resolve a hook value at mode activation time. Symbols are looked up once
   * and replaced with the callable they name; callables and NIL pass through.
   * A null (absent) value is treated as NIL.
   */
  Lisple::sptr_rtval resolve_hook(Lisple::Runtime& runtime, const Lisple::sptr_rtval& val)
  {
    if (!val || val->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
    if (val->type == Lisple::RTValue::Type::SYMBOL) return runtime.lookup_value(val->str());
    if (val->type == Lisple::RTValue::Type::FUNCTION) return val;
    return Lisple::Constant::NIL;
  }

  /**
   * Parse a Lisple vector of child entry maps into ChildSlot objects. Extracts
   * the structural fields (mode, id, state, width, height) and stores the full
   * entry map as slot.overrides so per-instance hook/style overrides are available
   * at build time without re-parsing here.
   */
  std::vector<Pixils::Runtime::ChildSlot> parse_child_slots(
    Lisple::Runtime& rt,
    const Lisple::sptr_rtval& children_val)
  {
    using namespace Pixils::Runtime;

    static Lisple::MapSchema child_schema(
      {{"mode", &Lisple::Type::SYMBOL}},
      {{"id", &Lisple::Type::ANY}, {"state", &Lisple::Type::ANY}});

    Lisple::Context ctx(rt);
    std::unordered_map<std::string, int> name_counts;
    std::vector<ChildSlot> slots;

    size_t n = Lisple::count(*children_val);
    for (size_t i = 0; i < n; i++)
    {
      auto child_entry = Lisple::get_child(*children_val, i);
      auto child_opts = child_schema.bind(ctx, *child_entry);

      ChildSlot slot;
      slot.mode_name = child_opts.val("mode")->str();

      if (child_opts.contains("id"))
        slot.id = child_opts.val("id")->str();
      else
      {
        int idx = name_counts[slot.mode_name]++;
        slot.id = slot.mode_name + "-" + std::to_string(idx);
      }

      slot.initial_state = child_opts.val("state");
      slot.overrides = child_entry;

      slots.push_back(std::move(slot));
    }
    return slots;
  }

  /**
   * Apply a Lisple override map onto an already-copied Mode in-place. Handles hooks,
   * style, layout direction, and children. Any key absent from the map is left at
   * whatever value the base mode copy carried. resolve_hook is called immediately so
   * symbol references are resolved against the current runtime state.
   */
  void apply_mode_overrides(Pixils::Runtime::Mode& mode,
                            const Lisple::sptr_rtval& overrides,
                            Lisple::Runtime& rt)
  {
    using namespace Pixils::Runtime;

    if (!overrides || overrides->type == Lisple::RTValue::Type::NIL) return;

    auto get = [&](const char* key) -> Lisple::sptr_rtval
    {
      auto val = Lisple::Dict::get_property(overrides, Lisple::RTValue::keyword(key));
      return val ? val : Lisple::Constant::NIL;
    };

    auto apply_hook = [&](Lisple::sptr_rtval& field, const char* key)
    {
      auto val = get(key);
      if (val->type != Lisple::RTValue::Type::NIL) field = resolve_hook(rt, val);
    };

    apply_hook(mode.init, "init");
    apply_hook(mode.update, "update");
    apply_hook(mode.render, "render");
    apply_hook(mode.on_mouse_down, "on-mouse-down");
    apply_hook(mode.on_mouse_up, "on-mouse-up");
    apply_hook(mode.on_mouse_enter, "on-mouse-enter");
    apply_hook(mode.on_mouse_leave, "on-mouse-leave");

    auto style_val = get("style");
    if (style_val->type != Lisple::RTValue::Type::NIL)
    {
      Lisple::Context ctx(rt);
      auto coercion = Pixils::Script::HostType::STYLE.coerce(ctx, style_val);
      if (coercion.success)
      {
        if (!mode.style) mode.style = Pixils::UI::Style{};
        Pixils::UI::apply_style_variant(*mode.style,
                                        Lisple::obj<Pixils::UI::Style>(*coercion.result));
      }
    }

    auto children_val = get("children");
    if (children_val->type != Lisple::RTValue::Type::NIL)
      mode.children = parse_child_slots(rt, children_val);
  }

} // namespace

namespace Pixils::Runtime
{

  void HookArguments::update_state(const Lisple::sptr_rtval& state)
  {
    this->init_args[0] = state;
    this->update_args[0] = state;
    this->render_args[0] = state;
  }

  Session::Session(Lisple::Runtime& lisple_runtime,
                   Asset::Registry& assets,
                   RenderContext& render_ctx,
                   const HookArguments& hook_args)
    : lisple_runtime(lisple_runtime)
    , assets(assets)
    , render_ctx(render_ctx)
    , mode_stack(lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK),
                 lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK_MESSAGES))
    , modes(lisple_runtime.lookup_value(Script::ID__PIXILS__MODES))
    , hook_args(hook_args)
  {
  }

  void Session::pop_mode()
  {
    if (mode_stack.size() > 1)
    {
      mode_stack.pop();

      /**
       * Restore active_mode from the context stack. Then re-sync
       * state from the Lisple stack (which holds the authoritative
       * saved state) and restore each child's state slice from that
       * parent state.
       */
      active_mode = ctx_stack.back();
      ctx_stack.pop_back();

      auto [_, saved_state] = mode_stack.peek();
      active_mode->state = saved_state;
      for (auto& child : active_mode->children)
      {
        restore_view_state(*child, active_mode->state);
      }

      this->hook_args.update_state(active_mode->state);
    }
  }

  void Session::push_mode(const Lisple::sptr_rtval& mode,
                          const Lisple::sptr_rtval& state,
                          const Lisple::sptr_rtval& overrides)
  {
    /**
     * Flush the current active context's state to the Lisple stack before pushing,
     * then save the context itself so pop_mode can recover it cheaply.
     */
    if (mode_stack.size() > 0)
    {
      mode_stack.update_state(active_mode->state);
      ctx_stack.push_back(std::move(active_mode));
    }

    this->mode_stack.push(mode, state);

    auto& mode_obj = Lisple::obj<Mode>(*mode);

    active_mode = std::make_shared<View>();
    active_mode->state = state;

    bool has_overrides = overrides && overrides->type != Lisple::RTValue::Type::NIL;

    if (has_overrides)
    {
      active_mode->owned_mode = std::make_unique<Mode>(mode_obj);
      apply_mode_overrides(*active_mode->owned_mode, overrides, lisple_runtime);
      active_mode->mode = active_mode->owned_mode.get();
    }
    else
    {
      active_mode->mode = &mode_obj;
    }

    /**
     * Resolve hooks in-place. For an owned copy this is safe; for a registry
     * entry it replaces symbols with callables once on first activation.
     */
    active_mode->mode->init = resolve_hook(lisple_runtime, active_mode->mode->init);
    active_mode->mode->update = resolve_hook(lisple_runtime, active_mode->mode->update);
    active_mode->mode->render = resolve_hook(lisple_runtime, active_mode->mode->render);
    active_mode->mode->on_mouse_down =
      resolve_hook(lisple_runtime, active_mode->mode->on_mouse_down);
    active_mode->mode->on_mouse_up =
      resolve_hook(lisple_runtime, active_mode->mode->on_mouse_up);
    active_mode->mode->on_mouse_enter =
      resolve_hook(lisple_runtime, active_mode->mode->on_mouse_enter);
    active_mode->mode->on_mouse_leave =
      resolve_hook(lisple_runtime, active_mode->mode->on_mouse_leave);

    if (!this->assets.is_loaded(active_mode->mode->name))
      this->assets.load(active_mode->mode->name, active_mode->mode->resources);

    this->hook_args.update_state(state);
    this->init_mode();

    /**
     * Build children and initialize each child, threading child states into
     * the parent state map as they complete.
     */
    auto parent_state = this->active_mode->state;
    for (const auto& slot : active_mode->mode->children)
    {
      this->active_mode->children.push_back(build_view(slot));
      parent_state = init_view(*this->active_mode->children.back(), parent_state);
    }
    this->active_mode->state = parent_state;
    this->hook_args.update_state(parent_state);
  }

  void Session::push_mode(const std::string& mode_name,
                          const Lisple::sptr_rtval& state,
                          const Lisple::sptr_rtval& overrides)
  {
    auto mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(mode_name));
    this->push_mode(mode, state, overrides);
  }

  void Session::process_messages()
  {
    Lisple::sptr_rtval_v messages = mode_stack.drain_messages();

    for (auto& message : messages)
    {
      std::string type =
        Lisple::Dict::get_property(message, Lisple::RTValue::keyword("type"))->str();

      if (type == "push")
      {
        mode_stack.update_state(active_mode->state);
        auto overrides_val =
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("overrides"));
        push_mode(
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("mode"))->str(),
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("state")),
          overrides_val ? overrides_val : Lisple::Constant::NIL);
      }
      else if (type == "pop")
      {
        pop_mode();
      }
    }
  }

  Lisple::sptr_rtval Session::invoke_hook(const Lisple::sptr_rtval& fn,
                                          Lisple::sptr_rtval_v& args,
                                          const Lisple::sptr_rtval& fallback)
  {
    if (!fn || fn->type == Lisple::RTValue::Type::NIL) return fallback;
    Lisple::Context exec_ctx(lisple_runtime);
    return fn->exec().execute(exec_ctx, args);
  }

  void Session::init_mode()
  {
    auto new_state = invoke_hook(this->active_mode->mode->init,
                                 this->hook_args.init_args,
                                 this->active_mode->state);
    this->active_mode->state = new_state;
    this->hook_args.update_state(new_state);
  }

  void Session::render_mode()
  {
    auto render_stack = mode_stack.get_render_stack();
    Rect full = {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h};

    /**
     * render_stack is top-first: [0]=top, [1]=just below, ..., [n-1]=bottom.
     * Render from bottom up, skipping index 0 (active_mode, rendered last).
     */
    for (size_t i = render_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      render_view(*this, *ctx_stack[ctx_idx], full);
    }

    render_view(*this, *active_mode, full);
  }

  std::shared_ptr<View> Session::build_view(const ChildSlot& slot)
  {
    auto mode_val =
      Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(slot.mode_name));
    Mode& base_mode = Lisple::obj<Mode>(*mode_val);

    View v;
    v.id = slot.id;
    v.state = Lisple::Constant::NIL;
    v.initial_state = slot.initial_state;

    bool has_overrides =
      slot.overrides && slot.overrides->type != Lisple::RTValue::Type::NIL;

    if (has_overrides)
    {
      v.owned_mode = std::make_unique<Mode>(base_mode);
      apply_mode_overrides(*v.owned_mode, slot.overrides, lisple_runtime);
      v.mode = v.owned_mode.get();
    }
    else
    {
      v.mode = &base_mode;
    }

    v.mode->init = resolve_hook(lisple_runtime, v.mode->init);
    v.mode->update = resolve_hook(lisple_runtime, v.mode->update);
    v.mode->render = resolve_hook(lisple_runtime, v.mode->render);
    v.mode->on_mouse_down = resolve_hook(lisple_runtime, v.mode->on_mouse_down);
    v.mode->on_mouse_up = resolve_hook(lisple_runtime, v.mode->on_mouse_up);
    v.mode->on_mouse_enter = resolve_hook(lisple_runtime, v.mode->on_mouse_enter);
    v.mode->on_mouse_leave = resolve_hook(lisple_runtime, v.mode->on_mouse_leave);

    for (const auto& grandchild_slot : v.mode->children)
      v.children.push_back(build_view(grandchild_slot));

    return std::make_shared<View>(std::move(v));
  }

} // namespace Pixils::Runtime
