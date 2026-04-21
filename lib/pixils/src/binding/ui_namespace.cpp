
#include "pixils/binding/ui_namespace.h"

#include <pixils/binding/point_namespace.h>
#include <pixils/ui/event.h>

#include <lisple/host/accessor.h>
#include <lisple/host/object.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  namespace Function
  {
    FUNC_IMPL(StopPropagation,
              SIG((FN_ARGS((&HostType::MOUSE_EVENT)),
                   EXEC_DISPATCH(&StopPropagation::exec_stop))));

    EXEC_BODY(StopPropagation, exec_stop)
    {
      Lisple::obj<MouseButtonEvent>(*args[0]).propagation_stopped = true;
      return Lisple::Constant::NIL;
    }
  } // namespace Function

  NATIVE_ADAPTER_IMPL(MouseEventAdapter,
                      MouseEvent,
                      &HostType::MOUSE_MOTION_EVENT,
                      (global_pos),
                      (local_pos))

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
    values.emplace("stop-propagation!", Function::StopPropagation::make());
  }

} // namespace Pixils::Script
