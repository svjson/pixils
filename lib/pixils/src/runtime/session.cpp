
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
   * Extract a context's state slice from the parent's state map at the given
   * keyword id. Returns NIL if parent is not a map or the key is absent.
   */
  Lisple::sptr_rtval extract_child_state(const Lisple::sptr_rtval& parent,
                                         const std::string& id)
  {
    if (!parent || parent->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
    auto val = Lisple::Dict::get_property(parent, Lisple::RTValue::keyword(id));
    return val ? val : Lisple::Constant::NIL;
  }

  /**
   * Store child_state into parent's state map under keyword id, returning the
   * updated parent. Creates a new empty map if parent is NIL.
   */
  Lisple::sptr_rtval merge_child_state(Lisple::sptr_rtval parent,
                                       const std::string& id,
                                       Lisple::sptr_rtval child_state)
  {
    auto key = Lisple::RTValue::keyword(id);
    if (!parent || parent->type == Lisple::RTValue::Type::NIL)
      parent = Lisple::RTValue::map({});
    Lisple::Dict::set_property(parent, key, child_state);
    return parent;
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

    static Lisple::MapSchema child_schema({{"mode", &Lisple::Type::SYMBOL}},
                                          {{"id", &Lisple::Type::ANY},
                                           {"state", &Lisple::Type::ANY},
                                           {"width", &Lisple::Type::NUMBER},
                                           {"height", &Lisple::Type::NUMBER}});

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

      if (child_opts.contains("width"))
        slot.width = DimensionConstraint::fixed(child_opts.i32("width"));
      if (child_opts.contains("height"))
        slot.height = DimensionConstraint::fixed(child_opts.i32("height"));

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

    auto style_val = get("style");
    if (style_val->type != Lisple::RTValue::Type::NIL)
    {
      Lisple::Context ctx(rt);
      auto coercion = Pixils::Script::HostType::STYLE.coerce(ctx, style_val);
      if (coercion.success)
      {
        mode.style = Lisple::obj<Pixils::UI::Style>(*coercion.result);
      }
    }

    auto layout_val = get("layout");
    if (layout_val->type != Lisple::RTValue::Type::NIL)
    {
      auto [ns, name] = layout_val->qual();
      mode.layout_direction =
        (name == "row") ? LayoutDirection::ROW : LayoutDirection::COLUMN;
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
      active_mode = std::move(ctx_stack.back());
      ctx_stack.pop_back();

      auto [_, saved_state] = mode_stack.peek();
      active_mode.state = saved_state;
      for (auto& child : active_mode.children)
      {
        restore_context_state(child, active_mode.state);
      }

      this->hook_args.update_state(active_mode.state);
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
      mode_stack.update_state(active_mode.state);
      ctx_stack.push_back(std::move(active_mode));
    }

    this->mode_stack.push(mode, state);

    auto& mode_obj = Lisple::obj<Mode>(*mode);

    active_mode = ModeContext{};
    active_mode.state = state;

    bool has_overrides = overrides && overrides->type != Lisple::RTValue::Type::NIL;

    if (has_overrides)
    {
      active_mode.owned_mode = std::make_unique<Mode>(mode_obj);
      apply_mode_overrides(*active_mode.owned_mode, overrides, lisple_runtime);
      active_mode.mode = active_mode.owned_mode.get();
    }
    else
    {
      active_mode.mode = &mode_obj;
    }

    /**
     * Resolve hooks in-place. For an owned copy this is safe; for a registry
     * entry it replaces symbols with callables once on first activation.
     */
    active_mode.mode->init = resolve_hook(lisple_runtime, active_mode.mode->init);
    active_mode.mode->update = resolve_hook(lisple_runtime, active_mode.mode->update);
    active_mode.mode->render = resolve_hook(lisple_runtime, active_mode.mode->render);
    active_mode.mode->on_mouse_down =
      resolve_hook(lisple_runtime, active_mode.mode->on_mouse_down);

    if (!this->assets.is_loaded(active_mode.mode->name))
      this->assets.load(active_mode.mode->name, active_mode.mode->resources);

    this->hook_args.update_state(state);
    this->init_mode();

    /**
     * Build children and initialize each child, threading child states into
     * the parent state map as they complete.
     */
    auto parent_state = this->active_mode.state;
    for (const auto& slot : active_mode.mode->children)
    {
      this->active_mode.children.push_back(build_mode_context(slot));
      parent_state = init_context(this->active_mode.children.back(), parent_state);
    }
    this->active_mode.state = parent_state;
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
        mode_stack.update_state(active_mode.state);
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
    auto new_state = invoke_hook(this->active_mode.mode->init,
                                 this->hook_args.init_args,
                                 this->active_mode.state);
    this->active_mode.state = new_state;
    this->hook_args.update_state(new_state);
  }

  void Session::update_mode()
  {
    /**
     * Snapshot any mouse button press from this frame into active_mouse_event so
     * update_context can route it through the component tree. The event lives for
     * exactly one call to update_mode and is cleared regardless of propagation outcome.
     */
    if (hook_args.events && hook_args.events->mouse_button_down &&
        hook_args.events->mouse_button_down->type != Lisple::RTValue::Type::NIL)
    {
      MouseEvent ev;
      ev.position = Lisple::obj<Point>(*hook_args.events->mouse_pos);
      ev.button = hook_args.events->mouse_button_down;
      active_mouse_event = ev;
    }
    else
    {
      active_mouse_event.reset();
    }

    auto update_stack = mode_stack.get_update_stack();

    /** Update composition modes below the top, preserving the existing offset semantics. */
    for (size_t i = update_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      auto& ctx = ctx_stack[ctx_idx];

      Lisple::sptr_rtval_v rargs = this->hook_args.update_args;
      rargs[0] = ctx.state;
      ctx.state = invoke_hook(ctx.mode->update, rargs, ctx.state);
      mode_stack.update_state(ctx.state, update_stack.size() - i);
    }

    /** Update the active (top) mode, then thread child updates through the parent state. */
    Lisple::sptr_rtval updated_state = invoke_hook(this->active_mode.mode->update,
                                                   this->hook_args.update_args,
                                                   this->active_mode.state);
    this->active_mode.state = updated_state;

    auto parent_state = this->active_mode.state;
    for (auto& child : this->active_mode.children)
      parent_state = update_context(child, parent_state);
    this->active_mode.state = parent_state;
    this->hook_args.update_state(parent_state);
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
      render_mode_context(*this, ctx_stack[ctx_idx], full);
    }

    render_mode_context(*this, active_mode, full);
  }

  ModeContext Session::build_mode_context(const ChildSlot& slot)
  {
    auto mode_val =
      Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(slot.mode_name));
    Mode& base_mode = Lisple::obj<Mode>(*mode_val);

    ModeContext ctx;
    ctx.id = slot.id;
    ctx.state = Lisple::Constant::NIL;
    ctx.initial_state = slot.initial_state;

    bool has_overrides =
      slot.overrides && slot.overrides->type != Lisple::RTValue::Type::NIL;

    if (has_overrides)
    {
      ctx.owned_mode = std::make_unique<Mode>(base_mode);
      apply_mode_overrides(*ctx.owned_mode, slot.overrides, lisple_runtime);
      ctx.mode = ctx.owned_mode.get();
    }
    else
    {
      ctx.mode = &base_mode;
    }

    ctx.mode->init = resolve_hook(lisple_runtime, ctx.mode->init);
    ctx.mode->update = resolve_hook(lisple_runtime, ctx.mode->update);
    ctx.mode->render = resolve_hook(lisple_runtime, ctx.mode->render);
    ctx.mode->on_mouse_down = resolve_hook(lisple_runtime, ctx.mode->on_mouse_down);

    for (const auto& grandchild_slot : ctx.mode->children)
      ctx.children.push_back(build_mode_context(grandchild_slot));

    return ctx;
  }

  Lisple::sptr_rtval Session::init_context(ModeContext& ctx,
                                           const Lisple::sptr_rtval& parent_state)
  {
    if (!this->assets.is_loaded(ctx.mode->name))
      this->assets.load(ctx.mode->name, ctx.mode->resources);

    ctx.state = extract_child_state(parent_state, ctx.id);
    if (ctx.state->type == Lisple::RTValue::Type::NIL) ctx.state = ctx.initial_state;

    Lisple::sptr_rtval_v iargs = {ctx.state, this->hook_args.init_args[1]};
    auto new_state = invoke_hook(ctx.mode->init, iargs);
    if (new_state->type != Lisple::RTValue::Type::NIL) ctx.state = new_state;

    for (auto& grandchild : ctx.children)
      ctx.state = init_context(grandchild, ctx.state);

    return merge_child_state(parent_state, ctx.id, ctx.state);
  }

  Lisple::sptr_rtval Session::update_context(ModeContext& ctx,
                                             const Lisple::sptr_rtval& parent_state)
  {
    ctx.state = extract_child_state(parent_state, ctx.id);

    /**
     * For styled components with known bounds, inject the current hover state
     * into the child's state map before the update hook fires. This allows
     * the component to react to hover in its update hook and lets the style
     * system select the correct variant in render.
     */
    if (hook_args.events && ctx.mode->style && ctx.bounds.w > 0)
    {
      const Point& mp = Lisple::obj<Point>(*hook_args.events->mouse_pos);
      int mx = mp.round_x();
      int my = mp.round_y();
      bool hovered = mx >= ctx.bounds.x && mx < ctx.bounds.x + ctx.bounds.w &&
                     my >= ctx.bounds.y && my < ctx.bounds.y + ctx.bounds.h;

      if (!ctx.state || ctx.state->type == Lisple::RTValue::Type::NIL)
        ctx.state = Lisple::RTValue::map({});
      Lisple::Dict::set_property(
        ctx.state,
        Lisple::RTValue::keyword("hovered"),
        hovered ? Lisple::Constant::BOOL_TRUE : Lisple::Constant::BOOL_FALSE);
    }

    Lisple::sptr_rtval_v uargs = {ctx.state, this->hook_args.update_args[1]};
    ctx.state = invoke_hook(ctx.mode->update, uargs, ctx.state);

    for (auto& grandchild : ctx.children)
      ctx.state = update_context(grandchild, ctx.state);

    /**
     * Fire on_mouse_down if a mouse button event occurred this frame, the component
     * has a handler, the cursor is within the component's last-rendered bounds, and
     * propagation has not already been stopped by a deeper component.
     */
    if (active_mouse_event && ctx.mode->on_mouse_down &&
        ctx.mode->on_mouse_down->type != Lisple::RTValue::Type::NIL &&
        !active_mouse_event->propagation_stopped && ctx.bounds.w > 0)
    {
      const Point& mp = active_mouse_event->position;
      int mx = mp.round_x();
      int my = mp.round_y();
      bool hit = mx >= ctx.bounds.x && mx < ctx.bounds.x + ctx.bounds.w &&
                 my >= ctx.bounds.y && my < ctx.bounds.y + ctx.bounds.h;
      if (hit)
      {
        auto ev_ref = Script::MouseEventAdapter::make_ref(*active_mouse_event);
        Lisple::sptr_rtval_v eargs = {ctx.state, ev_ref, this->hook_args.update_args[1]};
        auto new_state = invoke_hook(ctx.mode->on_mouse_down, eargs, ctx.state);
        if (new_state->type != Lisple::RTValue::Type::NIL) ctx.state = new_state;
      }
    }

    return merge_child_state(parent_state, ctx.id, ctx.state);
  }

  void Session::restore_context_state(ModeContext& ctx,
                                      const Lisple::sptr_rtval& parent_state)
  {
    ctx.state = extract_child_state(parent_state, ctx.id);
    for (auto& grandchild : ctx.children)
      restore_context_state(grandchild, ctx.state);
  }

} // namespace Pixils::Runtime
