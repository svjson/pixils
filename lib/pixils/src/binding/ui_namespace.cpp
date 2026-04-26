
#include "pixils/binding/ui_namespace.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/event.h>

#include <lisple/exec.h>
#include <lisple/host/accessor.h>
#include <lisple/host/object.h>
#include <lisple/runtime/dict.h>
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
      view.emit_event(
        CustomEvent{args[1], args.size() > 2 ? args[2] : Lisple::Constant::NIL});

      return Lisple::Constant::NIL;
    }

    /** StopPropagationFn - stop-propagation! */
    FUNC_IMPL(StopPropagation,
              SIG((FN_ARGS((&HostType::MOUSE_EVENT)),
                   EXEC_DISPATCH(&StopPropagation::exec_stop))));

    EXEC_BODY(StopPropagation, exec_stop)
    {
      Lisple::obj<MouseEvent>(*args[0]).propagation_stopped = true;
      return Lisple::Constant::NIL;
    }

  } // namespace Function

  NATIVE_ADAPTER_IMPL(BindStateAdapter, Runtime::BindState, &HostType::BIND_STATE);

  NATIVE_ADAPTER_IMPL(MouseEventAdapter,
                      MouseEvent,
                      &HostType::MOUSE_MOTION_EVENT,
                      ("global-position", global_pos),
                      ("position", local_pos))

  NOBJ_PROP_GET_ADAPTER__FIELD(MouseEventAdapter, global_pos, PointAdapter);
  NOBJ_PROP_GET_ADAPTER__FIELD(MouseEventAdapter, local_pos, PointAdapter);

  NATIVE_SUB_ADAPTER_IMPL(MouseEventAdapter,
                          MouseEvent,
                          (MouseButtonEventAdapter, MouseButtonEvent),
                          &HostType::MOUSE_EVENT,
                          (button))

  NOBJ_PROP_GET(MouseButtonEventAdapter, button)
  {
    return get_self_object().button;
  }

  UINamespace::UINamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__UI))
  {
    values.emplace("bind-state", Function::BindStateFn::make());
    values.emplace("emit!", Function::EmitBangFunction::make());
    values.emplace("stop-propagation!", Function::StopPropagation::make());
  }

} // namespace Pixils::Script
