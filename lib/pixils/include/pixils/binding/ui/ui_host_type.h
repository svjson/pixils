#ifndef PIXILS__BINDING__UI_HOST_TYPE_H
#define PIXILS__BINDING__UI_HOST_TYPE_H

#include <lisple/host.h>

namespace Pixils::Script::HostType
{
  HOST_TYPE(CUSTOM_EVENT, "HCustomEvent");
  HOST_TYPE(KEYBOARD_EVENT, "HKeyboardEvent");
  HOST_TYPE(MOUSE_EVENT, "HMouseEvent");
  HOST_TYPE(MOUSE_MOTION_EVENT, "HMouseMotionEvent");
  HOST_TYPE(DRAG_EVENT, "HDragEvent");
  HOST_TYPE(BIND_STATE, "HBindState");
} // namespace Pixils::Script::HostType

#endif /* UI_HOST_TYPE_H */
