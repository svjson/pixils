#include "pixils/ui/view_lifecycle.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/component_definition.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/component_state.h>
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

  Roo::sptr_val resolve_key_held_handler(Roo::Runtime& runtime, const Roo::sptr_val& val)
  {
    if (!val || val->type == Roo::Value::Type::NIL) return Roo::Constant::NIL;
    if (val->type == Roo::Value::Type::MAP) return val;
    return resolve_hook(runtime, val);
  }

  bool has_hook(const Roo::sptr_val& hook)
  {
    return hook && hook->type != Roo::Value::Type::NIL;
  }

  bool declared_component_ui_key(const Pixils::Runtime::View& view, const Roo::sptr_val& key)
  {
    if (!view.component || !key) return false;
    const auto& keys = view.component->ui_state_keys;
    return std::find(keys.begin(), keys.end(), key->str()) != keys.end();
  }

  Roo::sptr_val merge_declared_component_ui_state(const Pixils::Runtime::View& view,
                                                  const Roo::sptr_val& base,
                                                  const Roo::sptr_val& next)
  {
    auto result = base && base->type == Roo::Value::Type::MAP ? Roo::Dict::shallow_copy(base)
                                                              : Roo::map({});
    if (!next || next->type != Roo::Value::Type::MAP) return result;

    for (const auto& key : Roo::Dict::map_sptr_keys(next))
    {
      if (declared_component_ui_key(view, key))
      {
        Roo::Dict::set_property(result, key, Roo::Dict::get_property(next, key));
      }
    }
    return result;
  }

  Roo::sptr_val merge_ui_state_maps(const Roo::sptr_val& base, const Roo::sptr_val& next)
  {
    auto result = base && base->type == Roo::Value::Type::MAP ? Roo::Dict::shallow_copy(base)
                                                              : Roo::map({});
    if (!next || next->type != Roo::Value::Type::MAP) return result;

    for (const auto& key : Roo::Dict::map_sptr_keys(next))
    {
      Roo::Dict::set_property(result, key, Roo::Dict::get_property(next, key));
    }
    return result;
  }

  Roo::sptr_val invoke_ui_state_hook(Roo::Runtime& runtime,
                                     const std::shared_ptr<Pixils::Runtime::View>& view,
                                     const Roo::sptr_val& hook,
                                     const Roo::sptr_val& hook_ctx,
                                     const char* hook_name)
  {
    if (!has_hook(hook)) return view->ui_state;

    Roo::obj<Pixils::HookContext>(*hook_ctx).current_view = view;
    Roo::sptr_val_v args = {view->ui_state, view->state, hook_ctx};
    Roo::Context exec_ctx(runtime);
    auto result = hook->exec().execute(exec_ctx, args);
    auto next_ui_state =
      (result && result->type != Roo::Value::Type::NIL) ? result : view->ui_state;
    if (!next_ui_state || next_ui_state->type != Roo::Value::Type::MAP)
    {
      throw Roo::TypeError(std::string("Component :") + hook_name +
                           " must return a map or nil");
    }
    return next_ui_state;
  }

  void resolve_definition_hooks(Pixils::Runtime::ViewDefinition& definition,
                                Roo::Runtime& runtime)
  {
    definition.init = resolve_hook(runtime, definition.init);
    definition.update = resolve_hook(runtime, definition.update);
    definition.content_size = resolve_hook(runtime, definition.content_size);
    definition.render = resolve_hook(runtime, definition.render);
    definition.on_key_down = resolve_hook(runtime, definition.on_key_down);
    definition.on_key_held = resolve_key_held_handler(runtime, definition.on_key_held);
    definition.on_key_up = resolve_hook(runtime, definition.on_key_up);
    definition.on_click = resolve_hook(runtime, definition.on_click);
    definition.on_double_click = resolve_hook(runtime, definition.on_double_click);
    definition.on_mouse_down = resolve_hook(runtime, definition.on_mouse_down);
    definition.on_mouse_up = resolve_hook(runtime, definition.on_mouse_up);
    definition.on_mouse_enter = resolve_hook(runtime, definition.on_mouse_enter);
    definition.on_mouse_leave = resolve_hook(runtime, definition.on_mouse_leave);
    definition.on_mouse_motion = resolve_hook(runtime, definition.on_mouse_motion);
    definition.on_drag_start = resolve_hook(runtime, definition.on_drag_start);
    definition.on_drag = resolve_hook(runtime, definition.on_drag);
    definition.on_drag_end = resolve_hook(runtime, definition.on_drag_end);
    definition.on_drop = resolve_hook(runtime, definition.on_drop);
    if (definition.drag)
    {
      definition.drag->payload = resolve_hook(runtime, definition.drag->payload);
    }
  }

  void resolve_component_hooks(Pixils::Runtime::Component& component, Roo::Runtime& runtime)
  {
    component.init_ui = resolve_hook(runtime, component.init_ui);
    component.update_ui = resolve_hook(runtime, component.update_ui);
  }

  struct ResolvedViewDefinition
  {
    Pixils::Runtime::ViewDefinition* definition = nullptr;
    Pixils::Runtime::Mode* mode = nullptr;
    Pixils::Runtime::Component* component = nullptr;
  };

  ResolvedViewDefinition resolve_child_definition(const Pixils::Runtime::ChildSlot& slot,
                                                  const Roo::sptr_val& modes,
                                                  const Roo::sptr_val& components)
  {
    if (!slot.component_name.empty())
    {
      auto component_val =
        Roo::Dict::get_property(components, Roo::symbol(slot.component_name));
      if (!component_val || component_val->type == Roo::Value::Type::NIL)
      {
        throw Roo::InvocationException("Unknown child component '" + slot.component_name +
                                       "' referenced by child slot '" + slot.id + "'");
      }
      auto* component = Pixils::Script::component_from_registry_value(component_val);
      if (!component)
      {
        throw Roo::InvocationException("Child slot '" + slot.id + "' resolved component '" +
                                       slot.component_name + "' to non-component value");
      }
      return ResolvedViewDefinition{.definition = component,
                                    .mode = nullptr,
                                    .component = component};
    }

    auto mode_val = Roo::Dict::get_property(modes, Roo::symbol(slot.mode_name));
    if (mode_val && mode_val->type != Roo::Value::Type::NIL)
    {
      if (!Pixils::Script::HostType::MODE.is_type_of(*mode_val))
      {
        throw Roo::InvocationException("Child slot '" + slot.id + "' resolved mode '" +
                                       slot.mode_name + "' to non-mode value");
      }
      auto* mode = &Roo::obj<Pixils::Runtime::Mode>(*mode_val);
      return ResolvedViewDefinition{.definition = mode, .mode = mode, .component = nullptr};
    }

    auto component_val = Roo::Dict::get_property(components, Roo::symbol(slot.mode_name));
    if (!component_val || component_val->type == Roo::Value::Type::NIL)
    {
      throw Roo::InvocationException("Unknown child mode '" + slot.mode_name +
                                     "' referenced by child slot '" + slot.id + "'");
    }

    auto* component = Pixils::Script::component_from_registry_value(component_val);
    if (!component)
    {
      throw Roo::InvocationException("Child slot '" + slot.id + "' resolved mode '" +
                                     slot.mode_name + "' to non-component value");
    }
    return ResolvedViewDefinition{.definition = component,
                                  .mode = nullptr,
                                  .component = component};
  }

  void apply_definition_overrides(Pixils::Runtime::ViewDefinition& definition,
                                  Pixils::Runtime::Component* component,
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

    auto apply_component_hook = [&](Roo::sptr_val& field, const char* key)
    {
      auto val = get(key);
      if (val->type == Roo::Value::Type::NIL) return;
      if (!component)
      {
        throw Roo::TypeError(std::string("Child :") + key + " requires a component mode");
      }
      field = resolve_hook(runtime, val);
    };

    auto apply_key_held = [&](Roo::sptr_val& field, const char* key)
    {
      auto val = get(key);
      if (val->type != Roo::Value::Type::NIL) field = resolve_key_held_handler(runtime, val);
    };

    apply_hook(definition.init, "init");
    apply_hook(definition.update, "update");
    if (component)
    {
      apply_component_hook(component->init_ui, "init-ui");
      apply_component_hook(component->update_ui, "update-ui");
      apply_component_hook(component->after_layout_ui, "after-layout-ui");
    }
    else if (get("init-ui")->type != Roo::Value::Type::NIL ||
             get("update-ui")->type != Roo::Value::Type::NIL ||
             get("after-layout-ui")->type != Roo::Value::Type::NIL)
    {
      throw Roo::TypeError("Child UI hooks require a component mode");
    }
    apply_hook(definition.content_size, "content-size");
    apply_hook(definition.render, "render");
    apply_hook(definition.on_key_down, "on-key-down");
    apply_key_held(definition.on_key_held, "on-key-held");
    apply_hook(definition.on_key_up, "on-key-up");
    apply_hook(definition.on_mouse_down, "on-mouse-down");
    apply_hook(definition.on_mouse_up, "on-mouse-up");
    apply_hook(definition.on_click, "on-click");
    apply_hook(definition.on_double_click, "on-double-click");
    apply_hook(definition.on_mouse_enter, "on-mouse-enter");
    apply_hook(definition.on_mouse_leave, "on-mouse-leave");
    apply_hook(definition.on_mouse_motion, "on-mouse-motion");
    apply_hook(definition.on_mouse_wheel, "on-mouse-wheel");
    apply_hook(definition.on_drag_start, "on-drag-start");
    apply_hook(definition.on_drag, "on-drag");
    apply_hook(definition.on_drag_end, "on-drag-end");
    apply_hook(definition.on_drop, "on-drop");

    auto on_val = get("on");
    if (on_val->type == Roo::Value::Type::MAP)
    {
      for (auto& key : Roo::Dict::keys(*on_val))
      {
        auto handler = Roo::Dict::get_property(on_val, key);
        definition.event_handlers[key->str()] = resolve_hook(runtime, handler);
      }
    }

    auto style_val = get("style");
    if (style_val->type != Roo::Value::Type::NIL)
    {
      Roo::Context ctx(runtime);
      Pixils::Script::append_mode_style_layer(ctx, definition, style_val);
    }

    auto class_val = get("class");
    if (class_val->type != Roo::Value::Type::NIL)
    {
      append_class_names(definition.class_names,
                         Pixils::Script::parse_mode_classes(class_val));
    }

    auto drag_val = get("drag");
    if (drag_val->type != Roo::Value::Type::NIL)
    {
      Roo::Context ctx(runtime);
      definition.drag = Pixils::Script::parse_mode_drag_policy(ctx, drag_val);
    }

    auto focusable_val = get("focusable");
    if (focusable_val->type != Roo::Value::Type::NIL)
    {
      if (focusable_val->type != Roo::Value::Type::BOOL)
      {
        throw Roo::TypeError("Mode :focusable must be a boolean");
      }
      definition.focusable = std::get<bool>(focusable_val->value);
    }

    auto theme_val = get("theme");
    if (theme_val->type != Roo::Value::Type::NIL)
    {
      auto theme_names = Pixils::Script::parse_theme_names(theme_val, "Mode :theme");
      definition.theme =
        theme_names.empty() ? std::nullopt : std::make_optional(std::move(theme_names));
    }

    auto theme_variant_val = get("theme-variant");
    if (theme_variant_val->type != Roo::Value::Type::NIL)
    {
      definition.theme_variant =
        Pixils::Script::parse_theme_variant(theme_variant_val, "Mode :theme-variant");
    }

    auto children_val = get("children");
    if (children_val->type != Roo::Value::Type::NIL)
    {
      Roo::Context ctx(runtime);
      definition.children = Pixils::Script::parse_child_slots(ctx, children_val);
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
      apply_definition_overrides(*view.owned_mode, nullptr, overrides, runtime);
      view.definition = view.owned_mode.get();
      view.mode = view.owned_mode.get();
    }
    else
    {
      view.definition = &base_mode;
      view.mode = &base_mode;
    }

    resolve_definition_hooks(*view.definition, runtime);
  }

  std::shared_ptr<Runtime::View> build_view_tree(const Runtime::ChildSlot& slot,
                                                 const Roo::sptr_val& modes,
                                                 const Roo::sptr_val& components,
                                                 Roo::Runtime& runtime)
  {
    Runtime::View view;
    view.id = slot.id;
    view.source_mode_name = slot.mode_name;
    view.source_component_name = slot.component_name;
    view.state_binding = slot.state_binding;
    view.component_state_binding = slot.component_state_binding;
    view.ui_state_binding = slot.ui_state_binding;
    view.state = slot.initial_state;
    view.initial_state = slot.initial_state;
    view.state_policy = slot.state_policy.value_or("shared");

    if (slot.anonymous_mode)
    {
      view.owned_mode = std::make_unique<Runtime::Mode>(*slot.anonymous_mode);
      view.definition = view.owned_mode.get();
      view.mode = view.owned_mode.get();
      resolve_definition_hooks(*view.definition, runtime);
    }
    else
    {
      auto resolved = resolve_child_definition(slot, modes, components);
      if (resolved.component)
      {
        view.owned_component = std::make_unique<Runtime::Component>(*resolved.component);
        apply_definition_overrides(*view.owned_component,
                                   view.owned_component.get(),
                                   slot.overrides,
                                   runtime);
        view.definition = view.owned_component.get();
        view.component = view.owned_component.get();
        resolve_definition_hooks(*view.definition, runtime);
        resolve_component_hooks(*view.component, runtime);
      }
      else
      {
        view.owned_mode = std::make_unique<Runtime::Mode>(*resolved.mode);
        apply_definition_overrides(*view.owned_mode, nullptr, slot.overrides, runtime);
        view.definition = view.owned_mode.get();
        view.mode = view.owned_mode.get();
        resolve_definition_hooks(*view.definition, runtime);
      }
    }
    if (view.has_component_ui_state())
    {
      view.ui_state =
        merge_declared_component_ui_state(view, Roo::map({}), slot.initial_component_state);
      if (slot.has_initial_ui_state)
      {
        view.ui_state = merge_ui_state_maps(view.ui_state, slot.initial_ui_state);
      }
      view.initial_ui_state = view.ui_state;
    }
    else if (slot.has_initial_ui_state)
    {
      throw Roo::TypeError("Child :ui-state requires a component mode");
    }
    else if (slot.state_policy.has_value())
    {
      throw Roo::TypeError("Child :ui/state-policy requires a component mode");
    }
    if (!view.has_component_ui_state() && slot.has_component_state)
    {
      auto [binding, initial] = Runtime::parse_state_binding(slot.raw_state);
      view.state_binding = binding;
      view.component_state_binding = Roo::Constant::NIL;
      view.state = initial;
      view.initial_state = initial;
    }

    for (const auto& grandchild_slot : view.definition->children)
    {
      view.children.push_back(build_view_tree(grandchild_slot, modes, components, runtime));
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

    if (!assets.is_loaded(ctx.definition->name))
      assets.load(ctx.definition->name, ctx.definition->resources);

    ctx.set_state_if_changed(Runtime::extract_state(parent_state, ctx));

    Roo::sptr_val_v init_args = {ctx.state, init_hook_ctx};
    auto new_state = Runtime::invoke_hook(runtime, view, ctx.definition->init, init_args);
    if (new_state->type != Roo::Value::Type::NIL) ctx.set_state_if_changed(new_state);
    if (ctx.has_component_ui_state())
    {
      Roo::Context model_ctx(runtime);
      Runtime::transition_component_ui_state(
        ctx,
        Runtime::extract_component_ui_state(parent_state, ctx),
        model_ctx,
        true);
      ctx.seed_ui_state_from_shared_state_policy(runtime);
      auto next_ui_state = invoke_ui_state_hook(runtime,
                                                view,
                                                ctx.component->init_ui,
                                                init_hook_ctx,
                                                "init-ui");
      Runtime::transition_component_ui_state(ctx, next_ui_state, model_ctx);
      ctx.sync_state_from_shared_ui_state_policy();
    }

    for (auto& grandchild : ctx.children)
    {
      ctx.set_state_from_child_bindings(
        init_view_tree(assets, runtime, init_hook_ctx, grandchild, ctx.state),
        runtime);
      ctx.set_ui_state_from_child_bindings(*grandchild, runtime);
    }

    return Runtime::merge_component_ui_state(
      Runtime::merge_state(parent_state, ctx, ctx.state),
      ctx,
      ctx.ui_state);
  }

  void init_root_view(Asset::Registry& assets,
                      Roo::Runtime& runtime,
                      const Roo::sptr_val& init_hook_ctx,
                      const std::shared_ptr<Runtime::View>& view)
  {
    auto& ctx = *view;

    if (!assets.is_loaded(ctx.definition->name))
      assets.load(ctx.definition->name, ctx.definition->resources);

    Roo::sptr_val_v init_args = {ctx.state, init_hook_ctx};
    auto new_state =
      Runtime::invoke_hook(runtime, view, ctx.definition->init, init_args, ctx.state);
    ctx.set_state_if_changed(new_state);
    if (ctx.has_component_ui_state())
    {
      Roo::Context model_ctx(runtime);
      Runtime::transition_component_ui_state(
        ctx,
        Runtime::extract_component_ui_state(ctx.state, ctx),
        model_ctx,
        true);
      ctx.seed_ui_state_from_shared_state_policy(runtime);
      auto next_ui_state = invoke_ui_state_hook(runtime,
                                                view,
                                                ctx.component->init_ui,
                                                init_hook_ctx,
                                                "init-ui");
      Runtime::transition_component_ui_state(ctx, next_ui_state, model_ctx);
      ctx.sync_state_from_shared_ui_state_policy();
    }
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
