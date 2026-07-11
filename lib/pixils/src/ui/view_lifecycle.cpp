#include "pixils/ui/view_lifecycle.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/theme.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exception.h>
#include <roo/host/object.h>
#include <roo/host/schema.h>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>

namespace
{
  void append_class_names(std::vector<std::string>& target,
                          const std::vector<std::string>& classes)
  {
    for (const auto& class_name : classes)
    {
      if (std::find(target.begin(), target.end(), class_name) == target.end())
      {
        target.push_back(class_name);
      }
    }
  }

  Roo::sptr_val resolve_hook(Roo::Runtime& runtime, const Roo::sptr_val& val)
  {
    if (!val || val->type == Roo::Value::Type::NIL) return Roo::Constant::NIL;
    if (val->type == Roo::Value::Type::SYMBOL) return runtime.lookup(val->str());
    if (val->type == Roo::Value::Type::FUNCTION) return val;
    return Roo::Constant::NIL;
  }

  Roo::sptr_val resolve_key_held_handler(Roo::Runtime& runtime,
                                            const Roo::sptr_val& val)
  {
    if (!val || val->type == Roo::Value::Type::NIL) return Roo::Constant::NIL;
    if (val->type == Roo::Value::Type::MAP) return val;
    return resolve_hook(runtime, val);
  }

  void resolve_mode_hooks(Pixils::Runtime::Mode& mode, Roo::Runtime& runtime)
  {
    mode.init = resolve_hook(runtime, mode.init);
    mode.update = resolve_hook(runtime, mode.update);
    mode.content_size = resolve_hook(runtime, mode.content_size);
    mode.render = resolve_hook(runtime, mode.render);
    mode.on_key_down = resolve_hook(runtime, mode.on_key_down);
    mode.on_key_held = resolve_key_held_handler(runtime, mode.on_key_held);
    mode.on_key_up = resolve_hook(runtime, mode.on_key_up);
    mode.on_click = resolve_hook(runtime, mode.on_click);
    mode.on_double_click = resolve_hook(runtime, mode.on_double_click);
    mode.on_mouse_down = resolve_hook(runtime, mode.on_mouse_down);
    mode.on_mouse_up = resolve_hook(runtime, mode.on_mouse_up);
    mode.on_mouse_enter = resolve_hook(runtime, mode.on_mouse_enter);
    mode.on_mouse_leave = resolve_hook(runtime, mode.on_mouse_leave);
    mode.on_mouse_motion = resolve_hook(runtime, mode.on_mouse_motion);
    mode.on_drag_start = resolve_hook(runtime, mode.on_drag_start);
    mode.on_drag = resolve_hook(runtime, mode.on_drag);
    mode.on_drag_end = resolve_hook(runtime, mode.on_drag_end);
    mode.on_drop = resolve_hook(runtime, mode.on_drop);
    if (mode.drag)
    {
      mode.drag->payload = resolve_hook(runtime, mode.drag->payload);
    }
  }

  Pixils::Runtime::Mode& resolve_child_mode(const Pixils::Runtime::ChildSlot& slot,
                                            const Roo::sptr_val& modes)
  {
    auto mode_val = Roo::Dict::get_property(modes, Roo::symbol(slot.mode_name));
    if (!mode_val || mode_val->type == Roo::Value::Type::NIL)
    {
      throw Roo::InvocationException("Unknown child mode '" + slot.mode_name +
                                        "' referenced by child slot '" + slot.id + "'");
    }

    if (!Pixils::Script::HostType::MODE.is_type_of(*mode_val))
    {
      throw Roo::InvocationException("Child slot '" + slot.id + "' resolved mode '" +
                                        slot.mode_name + "' to non-mode value");
    }

    return Roo::obj<Pixils::Runtime::Mode>(*mode_val);
  }

  void apply_mode_overrides(Pixils::Runtime::Mode& mode,
                            const Roo::sptr_val& overrides,
                            Roo::Runtime& runtime)
  {
    if (!overrides || overrides->type == Roo::Value::Type::NIL) return;

    auto get = [&](const char* key) -> Roo::sptr_val
    {
      auto val = Roo::Dict::get_property(overrides, Roo::keyword(key));
      return val ? val : Roo::Constant::NIL;
    };

    auto apply_hook = [&](Roo::sptr_val& field, const char* key)
    {
      auto val = get(key);
      if (val->type != Roo::Value::Type::NIL) field = resolve_hook(runtime, val);
    };

    auto apply_key_held = [&](Roo::sptr_val& field, const char* key)
    {
      auto val = get(key);
      if (val->type != Roo::Value::Type::NIL)
        field = resolve_key_held_handler(runtime, val);
    };

    apply_hook(mode.init, "init");
    apply_hook(mode.update, "update");
    apply_hook(mode.content_size, "content-size");
    apply_hook(mode.render, "render");
    apply_hook(mode.on_key_down, "on-key-down");
    apply_key_held(mode.on_key_held, "on-key-held");
    apply_hook(mode.on_key_up, "on-key-up");
    apply_hook(mode.on_mouse_down, "on-mouse-down");
    apply_hook(mode.on_mouse_up, "on-mouse-up");
    apply_hook(mode.on_click, "on-click");
    apply_hook(mode.on_double_click, "on-double-click");
    apply_hook(mode.on_mouse_enter, "on-mouse-enter");
    apply_hook(mode.on_mouse_leave, "on-mouse-leave");
    apply_hook(mode.on_mouse_motion, "on-mouse-motion");
    apply_hook(mode.on_mouse_wheel, "on-mouse-wheel");
    apply_hook(mode.on_drag_start, "on-drag-start");
    apply_hook(mode.on_drag, "on-drag");
    apply_hook(mode.on_drag_end, "on-drag-end");
    apply_hook(mode.on_drop, "on-drop");

    auto on_val = get("on");
    if (on_val->type == Roo::Value::Type::MAP)
    {
      for (auto& key : Roo::Dict::keys(*on_val))
      {
        auto handler = Roo::Dict::get_property(on_val, key);
        mode.event_handlers[key->str()] = resolve_hook(runtime, handler);
      }
    }

    auto style_val = get("style");
    if (style_val->type != Roo::Value::Type::NIL)
    {
      Roo::Context ctx(runtime);
      Pixils::Script::append_mode_style_layer(ctx, mode, style_val);
    }

    auto class_val = get("class");
    if (class_val->type != Roo::Value::Type::NIL)
    {
      append_class_names(mode.class_names, Pixils::Script::parse_mode_classes(class_val));
    }

    auto focusable_val = get("focusable");
    if (focusable_val->type != Roo::Value::Type::NIL)
    {
      if (focusable_val->type != Roo::Value::Type::BOOL)
      {
        throw Roo::TypeError("Mode :focusable must be a boolean");
      }
      mode.focusable = std::get<bool>(focusable_val->value);
    }

    auto theme_val = get("theme");
    if (theme_val->type != Roo::Value::Type::NIL)
    {
      auto theme_names = Pixils::Script::parse_theme_names(theme_val, "Mode :theme");
      mode.theme =
        theme_names.empty() ? std::nullopt : std::make_optional(std::move(theme_names));
    }

    auto theme_variant_val = get("theme-variant");
    if (theme_variant_val->type != Roo::Value::Type::NIL)
    {
      mode.theme_variant = Pixils::Script::parse_theme_variant(theme_variant_val,
                                                               "Mode :theme-variant");
    }

    auto children_val = get("children");
    if (children_val->type != Roo::Value::Type::NIL)
    {
      Roo::Context ctx(runtime);
      mode.children = Pixils::Script::parse_child_slots(ctx, children_val);
    }
  }

} // namespace

namespace Pixils::UI
{
  std::shared_ptr<Runtime::View> build_root_view(Runtime::Mode& base_mode,
                                                 const Roo::sptr_val& state,
                                                 const Roo::sptr_val& overrides,
                                                 Roo::Runtime& runtime)
  {
    Runtime::View view;
    view.state = state;
    view.initial_state = state;

    attach_view_mode(view, base_mode, overrides, runtime);

    auto root = std::make_shared<Runtime::View>(std::move(view));
    attach_style_view_tree(root, nullptr);
    return root;
  }

  void attach_view_mode(Runtime::View& view,
                        Runtime::Mode& base_mode,
                        const Roo::sptr_val& overrides,
                        Roo::Runtime& runtime)
  {
    bool has_overrides = overrides && overrides->type != Roo::Value::Type::NIL;

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
                                                 const Roo::sptr_val& modes,
                                                 Roo::Runtime& runtime)
  {
    Runtime::View view;
    view.id = slot.id;
    view.state_binding = slot.state_binding;
    view.state = slot.initial_state;
    view.initial_state = slot.initial_state;

    if (slot.anonymous_mode)
    {
      view.owned_mode = std::make_unique<Runtime::Mode>(*slot.anonymous_mode);
      view.mode = view.owned_mode.get();
      resolve_mode_hooks(*view.mode, runtime);
    }
    else
    {
      auto& base_mode = resolve_child_mode(slot, modes);
      attach_view_mode(view, base_mode, slot.overrides, runtime);
    }

    for (const auto& grandchild_slot : view.mode->children)
    {
      view.children.push_back(build_view_tree(grandchild_slot, modes, runtime));
    }

    auto root = std::make_shared<Runtime::View>(std::move(view));
    attach_style_view_tree(root, nullptr);
    return root;
  }

  void attach_style_view_tree(const std::shared_ptr<Runtime::View>& view,
                              Runtime::View* parent)
  {
    if (!view) return;

    view->set_parent(parent);
    view->style_view.set_parent(parent ? &parent->style_view : nullptr);
    for (auto& child : view->children)
    {
      attach_style_view_tree(child, view.get());
    }
  }

  Roo::sptr_val init_view_tree(Asset::Registry& assets,
                                  Roo::Runtime& runtime,
                                  const Roo::sptr_val& init_hook_ctx,
                                  const std::shared_ptr<Runtime::View>& view,
                                  const Roo::sptr_val& parent_state)
  {
    auto& ctx = *view;

    if (!assets.is_loaded(ctx.mode->name)) assets.load(ctx.mode->name, ctx.mode->resources);

    ctx.set_state_if_changed(Runtime::extract_state(parent_state, ctx));

    Roo::sptr_val_v init_args = {ctx.state, init_hook_ctx};
    auto new_state = Runtime::invoke_hook(runtime, view, ctx.mode->init, init_args);
    if (new_state->type != Roo::Value::Type::NIL) ctx.set_state_if_changed(new_state);

    for (auto& grandchild : ctx.children)
    {
      ctx.set_state_if_changed(
        init_view_tree(assets, runtime, init_hook_ctx, grandchild, ctx.state));
    }

    return Runtime::merge_state(parent_state, ctx, ctx.state);
  }

  void init_root_view(Asset::Registry& assets,
                      Roo::Runtime& runtime,
                      const Roo::sptr_val& init_hook_ctx,
                      const std::shared_ptr<Runtime::View>& view)
  {
    auto& ctx = *view;

    if (!assets.is_loaded(ctx.mode->name)) assets.load(ctx.mode->name, ctx.mode->resources);

    Roo::sptr_val_v init_args = {ctx.state, init_hook_ctx};
    auto new_state =
      Runtime::invoke_hook(runtime, view, ctx.mode->init, init_args, ctx.state);
    ctx.set_state_if_changed(new_state);
  }

  void restore_view_tree(const std::shared_ptr<Runtime::View>& view,
                         const Roo::sptr_val& parent_state)
  {
    auto& ctx = *view;
    ctx.set_state_if_changed(Runtime::extract_state(parent_state, ctx));
    for (auto& grandchild : ctx.children)
    {
      restore_view_tree(grandchild, ctx.state);
    }
  }

} // namespace Pixils::UI
