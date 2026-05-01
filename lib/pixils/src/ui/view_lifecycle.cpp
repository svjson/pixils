#include "pixils/ui/view_lifecycle.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/style_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>

#include <lisple/context.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/host/schema.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>

namespace
{
  Lisple::sptr_rtval resolve_hook(Lisple::Runtime& runtime, const Lisple::sptr_rtval& val)
  {
    if (!val || val->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
    if (val->type == Lisple::RTValue::Type::SYMBOL) return runtime.lookup_value(val->str());
    if (val->type == Lisple::RTValue::Type::FUNCTION) return val;
    return Lisple::Constant::NIL;
  }

  void resolve_mode_hooks(Pixils::Runtime::Mode& mode, Lisple::Runtime& runtime)
  {
    mode.init = resolve_hook(runtime, mode.init);
    mode.update = resolve_hook(runtime, mode.update);
    mode.render = resolve_hook(runtime, mode.render);
    mode.on_click = resolve_hook(runtime, mode.on_click);
    mode.on_mouse_down = resolve_hook(runtime, mode.on_mouse_down);
    mode.on_mouse_up = resolve_hook(runtime, mode.on_mouse_up);
    mode.on_mouse_enter = resolve_hook(runtime, mode.on_mouse_enter);
    mode.on_mouse_leave = resolve_hook(runtime, mode.on_mouse_leave);
    mode.on_mouse_motion = resolve_hook(runtime, mode.on_mouse_motion);
  }

  void apply_mode_overrides(Pixils::Runtime::Mode& mode,
                            const Lisple::sptr_rtval& overrides,
                            Lisple::Runtime& runtime)
  {
    if (!overrides || overrides->type == Lisple::RTValue::Type::NIL) return;

    auto get = [&](const char* key) -> Lisple::sptr_rtval
    {
      auto val = Lisple::Dict::get_property(overrides, Lisple::RTValue::keyword(key));
      return val ? val : Lisple::Constant::NIL;
    };

    auto apply_hook = [&](Lisple::sptr_rtval& field, const char* key)
    {
      auto val = get(key);
      if (val->type != Lisple::RTValue::Type::NIL) field = resolve_hook(runtime, val);
    };

    apply_hook(mode.init, "init");
    apply_hook(mode.update, "update");
    apply_hook(mode.render, "render");
    apply_hook(mode.on_mouse_down, "on-mouse-down");
    apply_hook(mode.on_mouse_up, "on-mouse-up");
    apply_hook(mode.on_click, "on-click");
    apply_hook(mode.on_mouse_enter, "on-mouse-enter");
    apply_hook(mode.on_mouse_leave, "on-mouse-leave");
    apply_hook(mode.on_mouse_motion, "on-mouse-motion");

    auto style_val = get("style");
    if (style_val->type != Lisple::RTValue::Type::NIL)
    {
      Lisple::Context ctx(runtime);
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
    {
      Lisple::Context ctx(runtime);
      mode.children = Pixils::Script::parse_child_slots(ctx, children_val);
    }
  }

} // namespace

namespace Pixils::UI
{
  std::shared_ptr<Runtime::View> build_root_view(Runtime::Mode& base_mode,
                                                 const Lisple::sptr_rtval& state,
                                                 const Lisple::sptr_rtval& overrides,
                                                 Lisple::Runtime& runtime)
  {
    Runtime::View view;
    view.state = state;
    view.initial_state = state;

    attach_view_mode(view, base_mode, overrides, runtime);

    return std::make_shared<Runtime::View>(std::move(view));
  }

  void attach_view_mode(Runtime::View& view,
                        Runtime::Mode& base_mode,
                        const Lisple::sptr_rtval& overrides,
                        Lisple::Runtime& runtime)
  {
    bool has_overrides = overrides && overrides->type != Lisple::RTValue::Type::NIL;

    if (has_overrides)
    {
      view.owned_mode = std::make_unique<Runtime::Mode>(base_mode);
      apply_mode_overrides(*view.owned_mode, overrides, runtime);
      view.mode = view.owned_mode.get();
    }
    else
    {
      view.mode = &base_mode;
    }

    resolve_mode_hooks(*view.mode, runtime);
  }

  std::shared_ptr<Runtime::View> build_view_tree(const Runtime::ChildSlot& slot,
                                                 const Lisple::sptr_rtval& modes,
                                                 Lisple::Runtime& runtime)
  {
    auto mode_val =
      Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(slot.mode_name));
    auto& base_mode = Lisple::obj<Runtime::Mode>(*mode_val);

    Runtime::View view;
    view.id = slot.id;
    view.state_binding = slot.state_binding;
    view.state = slot.initial_state;
    view.initial_state = slot.initial_state;

    attach_view_mode(view, base_mode, slot.overrides, runtime);

    for (const auto& grandchild_slot : view.mode->children)
    {
      view.children.push_back(build_view_tree(grandchild_slot, modes, runtime));
    }

    return std::make_shared<Runtime::View>(std::move(view));
  }

  Lisple::sptr_rtval init_view_tree(Asset::Registry& assets,
                                    Lisple::Runtime& runtime,
                                    const Lisple::sptr_rtval& init_hook_ctx,
                                    const std::shared_ptr<Runtime::View>& view,
                                    const Lisple::sptr_rtval& parent_state)
  {
    auto& ctx = *view;

    if (!assets.is_loaded(ctx.mode->name)) assets.load(ctx.mode->name, ctx.mode->resources);

    ctx.state = Runtime::extract_state(parent_state, ctx);

    Lisple::sptr_rtval_v init_args = {ctx.state, init_hook_ctx};
    auto new_state = Runtime::invoke_hook(runtime, view, ctx.mode->init, init_args);
    if (new_state->type != Lisple::RTValue::Type::NIL) ctx.state = new_state;

    for (auto& grandchild : ctx.children)
    {
      ctx.state = init_view_tree(assets, runtime, init_hook_ctx, grandchild, ctx.state);
    }

    return Runtime::merge_state(parent_state, ctx, ctx.state);
  }

  void init_root_view(Asset::Registry& assets,
                      Lisple::Runtime& runtime,
                      const Lisple::sptr_rtval& init_hook_ctx,
                      const std::shared_ptr<Runtime::View>& view)
  {
    auto& ctx = *view;

    if (!assets.is_loaded(ctx.mode->name)) assets.load(ctx.mode->name, ctx.mode->resources);

    Lisple::sptr_rtval_v init_args = {ctx.state, init_hook_ctx};
    auto new_state =
      Runtime::invoke_hook(runtime, view, ctx.mode->init, init_args, ctx.state);
    ctx.state = new_state;
  }

  void restore_view_tree(const std::shared_ptr<Runtime::View>& view,
                         const Lisple::sptr_rtval& parent_state)
  {
    auto& ctx = *view;
    ctx.state = Runtime::extract_state(parent_state, ctx);
    for (auto& grandchild : ctx.children)
    {
      restore_view_tree(grandchild, ctx.state);
    }
  }

} // namespace Pixils::UI
