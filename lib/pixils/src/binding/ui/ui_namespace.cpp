
#include "pixils/binding/ui/ui_namespace.h"

#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/ui/style/style_host_type.h>
#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/event.h>
#include <pixils/ui/style.h>

#include <algorithm>
#include <roo/exception.h>
#include <roo/exec.h>
#include <roo/host/accessor.h>
#include <roo/host/object.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace Function
  {
    namespace
    {
      bool native_host_type_named(const Roo::sptr_val& value,
                                  const Roo::HostTypeRef& type_ref)
      {
        return value && value->type == Roo::Value::Type::NATIVE_OBJECT &&
               value->nobj()->get_host_type()->to_string() == type_ref.to_string();
      }

      Roo::sptr_val resolve_view_target(const Roo::sptr_val& target,
                                        const std::string& fn_name)
      {
        if (!target || target->type == Roo::Value::Type::NIL)
        {
          return Roo::Constant::NIL;
        }

        if (HostType::VIEW.is_type_of(*target) ||
            native_host_type_named(target, HostType::VIEW))
        {
          return target;
        }

        if (!Script::HostType::HOOK_CONTEXT.is_type_of(*target) &&
            !native_host_type_named(target, Script::HostType::HOOK_CONTEXT))
        {
          throw Roo::TypeError(fn_name + " target must be a view or hook context");
        }

        auto view = Roo::obj<HookContext>(*target).current_view;
        if (!view)
        {
          return Roo::Constant::NIL;
        }

        return ViewAdapter::make_ref(*view);
      }

      Runtime::Mode& ensure_instance_mode(Runtime::View& view)
      {
        if (!view.mode)
        {
          throw Roo::InvocationException("view has no mode");
        }

        if (!view.owned_mode)
        {
          view.owned_mode = std::make_unique<Runtime::Mode>(*view.mode);
          view.mode = view.owned_mode.get();
        }

        return *view.mode;
      }

      std::string theme_var_key(const Roo::sptr_val& key, const std::string& fn_name)
      {
        if (!key || (key->type != Roo::Value::Type::KEYWORD &&
                     key->type != Roo::Value::Type::SYMBOL))
        {
          throw Roo::TypeError(fn_name + " key must be a keyword or symbol");
        }

        return key->str();
      }

      void apply_runtime_style(Runtime::Mode& mode,
                               Roo::Context& ctx,
                               const Roo::sptr_val& style_val)
      {
        if (!style_val || style_val->type == Roo::Value::Type::NIL) return;

        if (contains_theme_var_ref(style_val))
        {
          mode.runtime_style_source = style_val;
          return;
        }

        UI::Style style;
        if (HostType::STYLE.is_type_of(*style_val))
        {
          style = Roo::obj<UI::Style>(*style_val);
        }
        else
        {
          auto mutable_style_val = style_val;
          auto coercion = HostType::STYLE.coerce(ctx, mutable_style_val);
          if (!coercion.success)
          {
            throw Roo::TypeError("ui/style! style argument must be a style map or style");
          }
          style = Roo::obj<UI::Style>(*coercion.result);
        }

        if (!mode.runtime_style)
        {
          mode.runtime_style = style;
        }
        else
        {
          UI::apply_style_variant(*mode.runtime_style, style);
        }

        if (!mode.style)
        {
          mode.style = style;
        }
        else
        {
          UI::apply_style_variant(*mode.style, style);
        }
      }

      bool disabled_view(const Runtime::View& view)
      {
        auto disabled = Roo::Dict::get_property(view.state, Roo::keyword("disabled?"));
        return disabled && disabled->type == Roo::Value::Type::BOOL &&
               std::get<bool>(disabled->value);
      }

      bool hidden_view(const Runtime::View& view)
      {
        return view.effective_style.visibility &&
               (*view.effective_style.visibility == UI::Style::Visibility::HIDDEN ||
                *view.effective_style.visibility == UI::Style::Visibility::NONE);
      }

      bool focus_candidate(const Runtime::View& view)
      {
        return view.mode && view.mode->focusable && !disabled_view(view) &&
               !hidden_view(view);
      }

      Runtime::View* find_descendant_mode(Runtime::View& view, const std::string& mode_name)
      {
        for (auto& child : view.children)
        {
          if (!child || !child->mode || hidden_view(*child)) continue;
          if (child->mode->name == mode_name) return child.get();
          if (auto found = find_descendant_mode(*child, mode_name)) return found;
        }
        return nullptr;
      }

      Runtime::View* find_first_focusable_descendant(Runtime::View& view)
      {
        for (auto& child : view.children)
        {
          if (!child || hidden_view(*child)) continue;
          if (focus_candidate(*child)) return child.get();
          if (auto found = find_first_focusable_descendant(*child)) return found;
        }
        return nullptr;
      }
    } // namespace

    /** BindStateFn - bind-state */
    FUNC_IMPL(BindStateFn,
              SIG((FN_ARGS((Roo::VARARG, &Roo::Type::ANY)),
                   EXEC_DISPATCH(&BindStateFn::exec_bind_state))));

    EXEC_BODY(BindStateFn, exec_bind_state)
    {
      return BindStateAdapter::make_unique(args);
    }

    /** ProjectStateFn - project-state */
    FUNC_IMPL(ProjectStateFn,
              SIG((FN_ARGS((Roo::VARARG, &Roo::Type::ANY)),
                   EXEC_DISPATCH(&ProjectStateFn::exec_project_state))));

    EXEC_BODY(ProjectStateFn, exec_project_state)
    {
      return BindStateAdapter::make_unique(Runtime::BindState(args, false));
    }

    /** BlurBangFunction - blur! */
    FUNC_IMPL(BlurBangFunction,
              MULTI_SIG((NO_ARGS, EXEC_DISPATCH(&BlurBangFunction::exec_blur)),
                        (FN_ARGS((&Script::HostType::HOOK_CONTEXT)),
                         EXEC_DISPATCH(&BlurBangFunction::exec_blur)),
                        (FN_ARGS((&HostType::VIEW)),
                         EXEC_DISPATCH(&BlurBangFunction::exec_blur))));

    EXEC_BODY(BlurBangFunction, exec_blur)
    {
      auto message_queue = ctx.lookup(ID__PIXILS__MODE_STACK_MESSAGES);
      auto target =
        args.empty() ? Roo::Constant::NIL : resolve_view_target(args[0], "ui/blur!");

      Roo::append(*message_queue,
                  Roo::map(Roo::sptr_val_v{
                    Roo::keyword("type"),
                    Roo::keyword("blur"),
                    Roo::keyword("target"),
                    target,
                  }));

      return Roo::Constant::NIL;
    }

    /** ChildrenFunction - children */
    FUNC_IMPL(ChildrenFunction,
              MULTI_SIG((FN_ARGS((&Script::HostType::HOOK_CONTEXT)),
                         EXEC_DISPATCH(&ChildrenFunction::exec_children)),
                        (FN_ARGS((&HostType::VIEW)),
                         EXEC_DISPATCH(&ChildrenFunction::exec_children))));

    EXEC_BODY(ChildrenFunction, exec_children)
    {
      auto target = resolve_view_target(args[0], "ui/children");
      if (!target || target->type == Roo::Value::Type::NIL)
      {
        return Roo::vector({});
      }

      Runtime::View& view = Roo::obj<Runtime::View>(*target);
      Roo::sptr_val_v children;
      children.reserve(view.children.size());
      for (const auto& child : view.children)
      {
        if (child) children.push_back(ViewAdapter::make_ref(*child));
      }

      return Roo::vector(children);
    }

    /** EmitFunction - emit */
    FUNC_IMPL(
      EmitBangFunction,
      MULTI_SIG((FN_ARGS((&Roo::Type::ANY), (&Roo::Type::KEYWORD), (&Roo::Type::ANY)),
                 EXEC_DISPATCH(&EmitBangFunction::exec_emit)),
                (FN_ARGS((&Roo::Type::ANY), (&Roo::Type::KEYWORD)),
                 EXEC_DISPATCH(&EmitBangFunction::exec_emit))))

    EXEC_BODY(EmitBangFunction, exec_emit)
    {
      auto target = resolve_view_target(args[0], "ui/emit!");
      if (!target || target->type == Roo::Value::Type::NIL)
      {
        return Roo::Constant::NIL;
      }

      Runtime::View& view = Roo::obj<Runtime::View>(*target);
      auto source_mode = view.mode ? Roo::symbol(view.mode->name) : Roo::Constant::NIL;
      view.emit_event(
        CustomEvent{args[1], args.size() > 2 ? args[2] : Roo::Constant::NIL, source_mode});

      return Roo::Constant::NIL;
    }

    /** FocusBangFunction - focus! */
    FUNC_IMPL(FocusBangFunction,
              MULTI_SIG((FN_ARGS((&Script::HostType::HOOK_CONTEXT)),
                         EXEC_DISPATCH(&FocusBangFunction::exec_focus)),
                        (FN_ARGS((&HostType::VIEW)),
                         EXEC_DISPATCH(&FocusBangFunction::exec_focus))));

    EXEC_BODY(FocusBangFunction, exec_focus)
    {
      auto target = resolve_view_target(args[0], "ui/focus!");
      auto message_queue = ctx.lookup(ID__PIXILS__MODE_STACK_MESSAGES);

      Roo::append(*message_queue,
                  Roo::map(Roo::sptr_val_v{
                    Roo::keyword("type"),
                    Roo::keyword("focus"),
                    Roo::keyword("target"),
                    target,
                  }));

      return target;
    }

    /** FocusFirstBangFunction - focus-first! */
    FUNC_IMPL(FocusFirstBangFunction,
              SIG((FN_ARGS((Roo::VARARG, &Roo::Type::ANY)),
                   EXEC_DISPATCH(&FocusFirstBangFunction::exec_focus_first))));

    EXEC_BODY(FocusFirstBangFunction, exec_focus_first)
    {
      if (args.empty() || args.size() > 2)
      {
        throw Roo::InvocationException(
          "ui/focus-first! expects a view or hook context and optional container mode");
      }

      auto root = resolve_view_target(args[0], "ui/focus-first!");
      if (!root || root->type == Roo::Value::Type::NIL)
      {
        return Roo::Constant::NIL;
      }

      Runtime::View* search_root = &Roo::obj<Runtime::View>(*root);
      if (args.size() > 1 && args[1] && args[1]->type != Roo::Value::Type::NIL)
      {
        if (args[1]->type != Roo::Value::Type::SYMBOL &&
            args[1]->type != Roo::Value::Type::KEYWORD &&
            args[1]->type != Roo::Value::Type::STRING)
        {
          throw Roo::TypeError(
            "ui/focus-first! container mode must be a symbol, keyword, or string");
        }
        search_root = find_descendant_mode(*search_root, args[1]->str());
        if (!search_root) return Roo::Constant::NIL;
      }

      auto target = find_first_focusable_descendant(*search_root);
      if (!target) return Roo::Constant::NIL;

      auto target_ref = ViewAdapter::make_ref(*target);
      auto message_queue = ctx.lookup(ID__PIXILS__MODE_STACK_MESSAGES);
      Roo::append(*message_queue,
                  Roo::map(Roo::sptr_val_v{
                    Roo::keyword("type"),
                    Roo::keyword("focus"),
                    Roo::keyword("target"),
                    target_ref,
                  }));

      return target_ref;
    }

    /** ReplaceChildBangFunction - replace-child! */
    FUNC_IMPL(ReplaceChildBangFunction,
              SIG((FN_ARGS((&Roo::Type::ANY), (&Roo::Type::ANY), (&Type::MAP_OR_STRING)),
                   EXEC_DISPATCH(&ReplaceChildBangFunction::exec_replace_child))));

    EXEC_BODY(ReplaceChildBangFunction, exec_replace_child)
    {
      auto target = resolve_view_target(args[0], "ui/replace-child!");
      if (!target || target->type == Roo::Value::Type::NIL)
      {
        return Roo::Constant::NIL;
      }

      Runtime::View& view = Roo::obj<Runtime::View>(*target);
      if (args[1]->type != Roo::Value::Type::STRING &&
          args[1]->type != Roo::Value::Type::SYMBOL &&
          args[1]->type != Roo::Value::Type::KEYWORD)
      {
        throw Roo::TypeError("ui/replace-child! child id must be string-like");
      }

      std::string child_id = args[1]->str();
      auto existing_child = std::find_if(view.children.begin(),
                                         view.children.end(),
                                         [&](const std::shared_ptr<Runtime::View>& child)
                                         { return child && child->id == child_id; });
      if (existing_child == view.children.end())
      {
        throw Roo::InvocationException("ui/replace-child! could not find child '" +
                                       child_id + "'");
      }

      auto child_entries = Roo::vector({args[2]});
      auto slots = parse_child_slots(ctx, child_entries);
      auto slot = std::move(slots.front());
      slot.id = child_id;
      if (slot.anonymous_mode && slot.anonymous_mode->selector_modes.empty())
      {
        slot.anonymous_mode->name = child_id;
      }
      view.queue_replace_child(child_id, std::move(slot));

      return Roo::Constant::NIL;
    }

    /** AppendChildBangFunction - append-child! */
    FUNC_IMPL(AppendChildBangFunction,
              SIG((FN_ARGS((&Roo::Type::ANY), (&Type::MAP_OR_STRING)),
                   EXEC_DISPATCH(&AppendChildBangFunction::exec_append_child))));

    EXEC_BODY(AppendChildBangFunction, exec_append_child)
    {
      auto target = resolve_view_target(args[0], "ui/append-child!");
      if (!target || target->type == Roo::Value::Type::NIL)
      {
        return Roo::Constant::NIL;
      }

      Runtime::View& view = Roo::obj<Runtime::View>(*target);
      auto child_entries = Roo::vector({args[1]});
      auto slots = parse_child_slots(ctx, child_entries);
      auto slot = std::move(slots.front());
      if (slot.anonymous_mode && slot.anonymous_mode->selector_modes.empty() &&
          !slot.id.empty())
      {
        slot.anonymous_mode->name = slot.id;
      }
      view.queue_append_child(std::move(slot));

      return Roo::Constant::NIL;
    }

    /** StyleBangFunction - style! */
    FUNC_IMPL(StyleBangFunction,
              SIG((FN_ARGS((&Roo::Type::ANY), (&Roo::Type::ANY)),
                   EXEC_DISPATCH(&StyleBangFunction::exec_style))));

    EXEC_BODY(StyleBangFunction, exec_style)
    {
      auto target = resolve_view_target(args[0], "ui/style!");
      if (!target || target->type == Roo::Value::Type::NIL)
      {
        return Roo::Constant::NIL;
      }

      Runtime::View& view = Roo::obj<Runtime::View>(*target);
      Runtime::Mode& mode = ensure_instance_mode(view);
      apply_runtime_style(mode, ctx, args[1]);
      view.style_view.invalidate();
      view.mark_style_changed();

      return target;
    }

    /** StopPropagationFn - stop-propagation! */
    FUNC_IMPL(StopPropagation,
              SIG((FN_ARGS((&HostType::EVENT)),
                   EXEC_DISPATCH(&StopPropagation::exec_stop))));

    EXEC_BODY(StopPropagation, exec_stop)
    {
      Roo::obj<Event>(*args[0]).propagation_stopped = true;
      return Roo::Constant::NIL;
    }

    /** PreserveFocusBangFunction - preserve-focus! */
    FUNC_IMPL(PreserveFocusBangFunction,
              SIG((FN_ARGS((&HostType::EVENT)),
                   EXEC_DISPATCH(&PreserveFocusBangFunction::exec_preserve_focus))));

    EXEC_BODY(PreserveFocusBangFunction, exec_preserve_focus)
    {
      Roo::obj<Event>(*args[0]).preserve_focus = true;
      return Roo::Constant::NIL;
    }

    /** ActiveThemeVarFunction - theme-var */
    FUNC_IMPL(ActiveThemeVarFunction,
              SIG((FN_ARGS((Roo::VARARG, &Roo::Type::ANY)),
                   EXEC_DISPATCH(&ActiveThemeVarFunction::exec_theme_var))));

    EXEC_BODY(ActiveThemeVarFunction, exec_theme_var)
    {
      if (args.size() < 2 || args.size() > 3)
      {
        throw Roo::InvocationException(
          "ui/theme-var expects a view or hook context, a key, and optional fallback");
      }

      const auto fallback = args.size() > 2 ? args[2] : Roo::Constant::NIL;
      auto target = resolve_view_target(args[0], "ui/theme-var");
      if (!target || target->type == Roo::Value::Type::NIL)
      {
        return fallback;
      }

      Runtime::View& view = Roo::obj<Runtime::View>(*target);
      auto raw_value = lookup_theme_var(view.effective_theme,
                                        view.effective_theme.selected_variant,
                                        theme_var_key(args[1], "ui/theme-var"));
      if (!raw_value)
      {
        return fallback;
      }

      auto resolved_value = resolve_theme_vars(view.effective_theme,
                                               view.effective_theme.selected_variant,
                                               raw_value);
      return resolved_value ? resolved_value : fallback;
    }

  } // namespace Function

  NATIVE_ADAPTER_IMPL(BindStateAdapter, Runtime::BindState, &HostType::BIND_STATE);

  NATIVE_ADAPTER_IMPL(EventAdapter, Event, &HostType::EVENT);

  NATIVE_SUB_ADAPTER_IMPL(EventAdapter,
                          Event,
                          (CustomEventAdapter, CustomEvent),
                          &HostType::CUSTOM_EVENT,
                          ("event-key", event_key),
                          ("source-mode", source_mode),
                          (payload))

  NOBJ_PROP_GET(CustomEventAdapter, event_key)
  {
    return get_self_object().event_key;
  }

  NOBJ_PROP_GET(CustomEventAdapter, payload)
  {
    return get_self_object().payload;
  }

  NOBJ_PROP_GET(CustomEventAdapter, source_mode)
  {
    return get_self_object().source_mode;
  }

  NATIVE_SUB_ADAPTER_IMPL(EventAdapter,
                          Event,
                          (MouseEventAdapter, MouseEvent),
                          &HostType::MOUSE_MOTION_EVENT,
                          ("global-position", global_pos),
                          ("position", local_pos))

  NOBJ_PROP_GET(MouseEventAdapter, global_pos)
  {
    const Point& point = get_self_object().global_pos;
    return PointAdapter::make_unique(point.x, point.y);
  }

  NOBJ_PROP_GET(MouseEventAdapter, local_pos)
  {
    const Point& point = get_self_object().local_pos;
    return PointAdapter::make_unique(point.x, point.y);
  }

  NATIVE_SUB_ADAPTER_IMPL(MouseEventAdapter,
                          MouseEvent,
                          (MouseWheelEventAdapter, MouseWheelEvent),
                          &HostType::MOUSE_WHEEL_EVENT,
                          (delta),
                          (x),
                          (y))

  NOBJ_PROP_GET(MouseWheelEventAdapter, delta)
  {
    const Point& point = get_self_object().delta;
    return PointAdapter::make_unique(point.x, point.y);
  }

  NOBJ_PROP_GET(MouseWheelEventAdapter, x)
  {
    return Roo::number(get_self_object().delta.x);
  }

  NOBJ_PROP_GET(MouseWheelEventAdapter, y)
  {
    return Roo::number(get_self_object().delta.y);
  }

  NATIVE_SUB_ADAPTER_IMPL(MouseEventAdapter,
                          MouseEvent,
                          (MouseButtonEventAdapter, MouseButtonEvent),
                          &HostType::MOUSE_EVENT,
                          (button),
                          ("click-count", click_count))

  NOBJ_PROP_GET(MouseButtonEventAdapter, button)
  {
    return get_self_object().button;
  }

  NOBJ_PROP_GET(MouseButtonEventAdapter, click_count)
  {
    return Roo::number(get_self_object().click_count);
  }

  NATIVE_SUB_ADAPTER_IMPL(MouseButtonEventAdapter,
                          MouseButtonEvent,
                          (DragEventAdapter, DragEvent),
                          &HostType::DRAG_EVENT,
                          ("start-global-position", start_global_pos),
                          ("start-position", start_local_pos),
                          (delta),
                          ("total-delta", total_delta),
                          (payload))

  NOBJ_PROP_GET(DragEventAdapter, start_global_pos)
  {
    const Point& point = get_self_object().start_global_pos;
    return PointAdapter::make_unique(point.x, point.y);
  }

  NOBJ_PROP_GET(DragEventAdapter, start_local_pos)
  {
    const Point& point = get_self_object().start_local_pos;
    return PointAdapter::make_unique(point.x, point.y);
  }

  NOBJ_PROP_GET(DragEventAdapter, delta)
  {
    const Point& point = get_self_object().delta;
    return PointAdapter::make_unique(point.x, point.y);
  }

  NOBJ_PROP_GET(DragEventAdapter, total_delta)
  {
    const Point& point = get_self_object().total_delta;
    return PointAdapter::make_unique(point.x, point.y);
  }

  NOBJ_PROP_GET(DragEventAdapter, payload)
  {
    return get_self_object().payload;
  }

  NATIVE_SUB_ADAPTER_IMPL(EventAdapter,
                          Event,
                          (KeyboardEventAdapter, KeyboardEvent),
                          &HostType::KEYBOARD_EVENT,
                          (key),
                          ("held-keys", held_keys),
                          (match))

  NOBJ_PROP_GET(KeyboardEventAdapter, key)
  {
    return get_self_object().key;
  }

  NOBJ_PROP_GET(KeyboardEventAdapter, held_keys)
  {
    return get_self_object().held_keys;
  }

  NOBJ_PROP_GET(KeyboardEventAdapter, match)
  {
    return get_self_object().match;
  }

  UINamespace::UINamespace()
    : Roo::Namespace(std::string(NS__PIXILS__UI))
  {
    values.emplace("bind-state", Function::BindStateFn::make());
    values.emplace("project-state", Function::ProjectStateFn::make());
    values.emplace(FN__PIXILS__UI__BLUR_BANG, Function::BlurBangFunction::make());
    values.emplace("append-child!", Function::AppendChildBangFunction::make());
    values.emplace(FN__PIXILS__UI__CHILDREN, Function::ChildrenFunction::make());
    values.emplace("emit!", Function::EmitBangFunction::make());
    values.emplace(FN__PIXILS__UI__FOCUS_BANG, Function::FocusBangFunction::make());
    values.emplace(FN__PIXILS__UI__FOCUS_FIRST_BANG,
                   Function::FocusFirstBangFunction::make());
    values.emplace("replace-child!", Function::ReplaceChildBangFunction::make());
    values.emplace(FN__PIXILS__UI__STYLE_BANG, Function::StyleBangFunction::make());
    values.emplace("preserve-focus!", Function::PreserveFocusBangFunction::make());
    values.emplace("stop-propagation!", Function::StopPropagation::make());
    values.emplace(FN__PIXILS__UI__THEME_VAR, Function::ActiveThemeVarFunction::make());
  }

} // namespace Pixils::Script
