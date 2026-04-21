
#ifndef PIXILS__UI__EVENT_H
#define PIXILS__UI__EVENT_H

#include <pixils/geom.h>

#include <lisple/runtime/value.h>

namespace Pixils
{
  /**
   * Base event type. Owns the propagation lifecycle - the C++ orchestration
   * code creates the event, passes it to hooks via a Lisple reference, and
   * reads propagation_stopped after each hook returns to decide whether to
   * continue bubbling up the component tree.
   */
  struct Event
  {
    bool propagation_stopped = false;
    virtual ~Event() = default;
  };

  /**
   * Mouse motion event. Carries the global (buffer) and local (component)
   * cursor positions. Used directly for on_mouse_enter and on_mouse_leave;
   * local_pos may be just outside component bounds on leave.
   */
  struct MouseEvent : Event
  {
    Point global_pos;
    Point local_pos;
  };

  /**
   * Mouse button event. Extends MouseEvent with the button that was
   * pressed or released, as a Lisple keyword (:left, :right, :middle).
   */
  struct MouseButtonEvent : MouseEvent
  {
    Lisple::sptr_rtval button = Lisple::Constant::NIL;
  };

} // namespace Pixils

#endif /* PIXILS__UI__EVENT_H */
