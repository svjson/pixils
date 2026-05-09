
#include "pixils/binding/ui/ui_namespace.h"

#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/event.h>

#include <algorithm>
#include <lisple/exception.h>
#include <lisple/exec.h>
#include <lisple/host/accessor.h>
#include <lisple/host/object.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  namespace Function
  {
    /** BindStateFn - bind-state */
    FUNC_IMPL(BindStateFn,
              SIG((FN_ARGS((Lisple::VARARG, &Lisple::Type::ANY)),
                   EXEC_DISPATCH(&BindStateFn::exec_bind_state))));

    EXEC_BODY(BindStateFn, exec_bind_state)
    {
      return BindStateAdapter::make_unique(args);
    }

    /** EmitFunction - emit */
    FUNC_IMPL(
      EmitBangFunction,
      MULTI_SIG((FN_ARGS((&HostType::VIEW), (&Lisple::Type::KEY), (&Lisple::Type::ANY)),
                 EXEC_DISPATCH(&EmitBangFunction::exec_emit)),
                (FN_ARGS((&HostType::VIEW), (&Lisple::Type::KEY)),
                 EXEC_DISPATCH(&EmitBangFunction::exec_emit))))

    EXEC_BODY(EmitBangFunction, exec_emit)
    {
      Runtime::View& view = Lisple::obj<Runtime::View>(*args[0]);
      auto source_mode =
        view.mode ? Lisple::RTValue::symbol(view.mode->name) : Lisple::Constant::NIL;
      view.emit_event(CustomEvent{args[1],
                                  args.size() > 2 ? args[2] : Lisple::Constant::NIL,
                                  source_mode});

      return Lisple::Constant::NIL;
    }

    /** ReplaceChildBangFunction - replace-child! */
    FUNC_IMPL(ReplaceChildBangFunction,
              SIG((FN_ARGS((&HostType::VIEW), (&Lisple::Type::ANY), (&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&ReplaceChildBangFunction::exec_replace_child))));

    EXEC_BODY(ReplaceChildBangFunction, exec_replace_child)
    {
      Runtime::View& view = Lisple::obj<Runtime::View>(*args[0]);
      if (args[1]->type != Lisple::RTValue::Type::STRING &&
          args[1]->type != Lisple::RTValue::Type::SYMBOL &&
          args[1]->type != Lisple::RTValue::Type::KEYWORD)
      {
        throw Lisple::TypeError("ui/replace-child! child id must be string-like");
      }

      std::string child_id = args[1]->str();
      auto existing_child = std::find_if(view.children.begin(),
                                         view.children.end(),
                                         [&](const std::shared_ptr<Runtime::View>& child)
                                         { return child && child->id == child_id; });
      if (existing_child == view.children.end())
      {
        throw Lisple::InvocationException("ui/replace-child! could not find child '" +
                                          child_id + "'");
      }

      auto child_entries = Lisple::RTValue::vector({args[2]});
      auto slots = parse_child_slots(ctx, child_entries);
      auto slot = std::move(slots.front());
      slot.id = child_id;
      view.queue_replace_child(child_id, std::move(slot));

      return Lisple::Constant::NIL;
    }

    /** StopPropagationFn - stop-propagation! */
    FUNC_IMPL(StopPropagation,
              MULTI_SIG((FN_ARGS((&HostType::MOUSE_EVENT)),
                         EXEC_DISPATCH(&StopPropagation::exec_stop)),
                        (FN_ARGS((&HostType::CUSTOM_EVENT)),
                         EXEC_DISPATCH(&StopPropagation::exec_stop))));

    EXEC_BODY(StopPropagation, exec_stop)
    {
      if (HostType::MOUSE_EVENT.is_type_of(*args[0]))
      {
        Lisple::obj<MouseEvent>(*args[0]).propagation_stopped = true;
      }
      else
      {
        Lisple::obj<CustomEvent>(*args[0]).propagation_stopped = true;
      }
      return Lisple::Constant::NIL;
    }

  } // namespace Function

  NATIVE_ADAPTER_IMPL(BindStateAdapter, Runtime::BindState, &HostType::BIND_STATE);

  NATIVE_ADAPTER_IMPL(CustomEventAdapter,
                      CustomEvent,
                      &HostType::CUSTOM_EVENT,
                      ("event-key", event_key),
                      ("source-mode", source_mode),
                      (payload))

  NOBJ_PROP_GET(CustomEventAdapter, event_key)
  {
    return get_object().event_key;
  }

  NOBJ_PROP_GET(CustomEventAdapter, payload)
  {
    return get_object().payload;
  }

  NOBJ_PROP_GET(CustomEventAdapter, source_mode)
  {
    return get_object().source_mode;
  }

  NATIVE_ADAPTER_IMPL(MouseEventAdapter,
                      MouseEvent,
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
                          (MouseButtonEventAdapter, MouseButtonEvent),
                          &HostType::MOUSE_EVENT,
                          (button))

  NOBJ_PROP_GET(MouseButtonEventAdapter, button)
  {
    return get_self_object().button;
  }

  NATIVE_SUB_ADAPTER_IMPL(MouseButtonEventAdapter,
                          MouseButtonEvent,
                          (DragEventAdapter, DragEvent),
                          &HostType::DRAG_EVENT,
                          ("start-global-position", start_global_pos),
                          ("start-position", start_local_pos),
                          (delta),
                          ("total-delta", total_delta))

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

  NATIVE_ADAPTER_IMPL(KeyboardEventAdapter,
                      KeyboardEvent,
                      &HostType::KEYBOARD_EVENT,
                      (key),
                      ("held-keys", held_keys),
                      (match))

  NOBJ_PROP_GET(KeyboardEventAdapter, key)
  {
    return get_object().key;
  }

  NOBJ_PROP_GET(KeyboardEventAdapter, held_keys)
  {
    return get_object().held_keys;
  }

  NOBJ_PROP_GET(KeyboardEventAdapter, match)
  {
    return get_object().match;
  }

  UINamespace::UINamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__UI))
  {
    values.emplace("bind-state", Function::BindStateFn::make());
    values.emplace("emit!", Function::EmitBangFunction::make());
    values.emplace("replace-child!", Function::ReplaceChildBangFunction::make());
    values.emplace("stop-propagation!", Function::StopPropagation::make());
  }

} // namespace Pixils::Script
