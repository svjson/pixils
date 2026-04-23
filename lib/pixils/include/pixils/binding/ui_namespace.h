
#ifndef PIXILS__BINDING__UI_NAMESPACE_H
#define PIXILS__BINDING__UI_NAMESPACE_H

#include <pixils/ui/event.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>

namespace Pixils::Runtime
{
  struct BindState;
}

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__UI = "pixils.ui";

  namespace HostType
  {
    HOST_TYPE(MOUSE_EVENT, "HMouseEvent");
    HOST_TYPE(MOUSE_MOTION_EVENT, "HMouseMotionEvent");
    HOST_TYPE(BIND_STATE, "HBindState");
  } // namespace HostType

  namespace Function
  {
    FUNC(BindStateFn, bind_state);
    FUNC(EmitBangFunction, emit);
    FUNC(StopPropagation, stop);
  } // namespace Function

  NATIVE_ADAPTER(MouseEventAdapter, MouseEvent, (global_pos, local_pos));
  NATIVE_SUB_ADAPTER(MouseEventAdapter,
                     (MouseButtonEventAdapter, MouseButtonEvent),
                     (button));
  NATIVE_ADAPTER(BindStateAdapter, Runtime::BindState);

  class UINamespace : public Lisple::Namespace
  {
   public:
    UINamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__BINDING__UI_NAMESPACE_H */
