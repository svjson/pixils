
#include "pixils/binding/ui_namespace.h"

#include <pixils/binding/point_namespace.h>
#include <pixils/ui/event.h>

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
      Lisple::obj<MouseEvent>(*args[0]).propagation_stopped = true;
      return Lisple::Constant::NIL;
    }
  } // namespace Function

  NATIVE_ADAPTER_IMPL(MouseEventAdapter,
                      MouseEvent,
                      &HostType::MOUSE_EVENT,
                      (position),
                      (button))

  NOBJ_PROP_GET(MouseEventAdapter, position)
  {
    return PointAdapter::make_ref(get_self_object().position);
  }

  NOBJ_PROP_GET(MouseEventAdapter, button)
  {
    return get_self_object().button ? get_self_object().button : Lisple::Constant::NIL;
  }

  UINamespace::UINamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__UI))
  {
    values.emplace("stop-propagation!", Function::StopPropagation::make());
  }

} // namespace Pixils::Script
