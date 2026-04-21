
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
  };

  /**
   * Mouse button event. Carries the cursor position at the time of the
   * button press and which button was pressed as a Lisple keyword
   * (:left, :right, :middle).
   */
  struct MouseEvent : Event
  {
    enum class Type : uint8_t
    {
      MOUSE_DOWN,
      MOUSE_UP
    };

    Type type;
    Point position;
    Lisple::sptr_rtval button = Lisple::Constant::NIL;
  };

} // namespace Pixils

#endif /* PIXILS__UI__EVENT_H */
