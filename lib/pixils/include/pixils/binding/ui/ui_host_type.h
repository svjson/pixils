#ifndef PIXILS__BINDING__UI_HOST_TYPE_H
#define PIXILS__BINDING__UI_HOST_TYPE_H

#include <lisple/host/type.h>

namespace Pixils::Script::HostType
{
  HOST_TYPE(EVENT, "HEvent");
  HOST_SUB_TYPE(CUSTOM_EVENT, "HCustomEvent", &EVENT);
  HOST_SUB_TYPE(KEYBOARD_EVENT, "HKeyboardEvent", &EVENT);
  HOST_SUB_TYPE(MOUSE_MOTION_EVENT, "HMouseMotionEvent", &EVENT);
  HOST_SUB_TYPE(MOUSE_EVENT, "HMouseEvent", &EVENT);
  HOST_SUB_TYPE(DRAG_EVENT, "HDragEvent", &EVENT);
  HOST_TYPE(BIND_STATE, "HBindState");
} // namespace Pixils::Script::HostType

#endif /* UI_HOST_TYPE_H */
