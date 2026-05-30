
#ifndef PIXILS__UI__EVENT_H
#define PIXILS__UI__EVENT_H

#include <pixils/geom.h>

#include <roo/runtime/value.h>

namespace Pixils
{
  /**
   * Base event type. Owns the propagation lifecycle - the C++ orchestration
   * code creates the event, passes it to hooks via a Roo reference, and
   * reads propagation_stopped after each hook returns to decide whether to
   * continue bubbling up the component tree.
   */
  struct Event
  {
    bool propagation_stopped = false;
    virtual ~Event() = default;
  };

  struct CustomEvent : public Event
  {
    Roo::sptr_val event_key;
    Roo::sptr_val source_mode = Roo::Constant::NIL;
    Roo::sptr_val payload;

    CustomEvent(const Roo::sptr_val& event_key,
                const Roo::sptr_val& payload,
                const Roo::sptr_val& source_mode = Roo::Constant::NIL);
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
   * pressed or released, as a Roo keyword (:left, :right, :middle).
   */
  struct MouseButtonEvent : MouseEvent
  {
    Roo::sptr_val button = Roo::Constant::NIL;
    uint8_t click_count = 1;
  };

  /**
   * Mouse drag event routed to the view chain that received the initiating
   * mouse-down. `delta` is movement since the previous drag lifecycle event for
   * this button, while `total_delta` is movement since the initiating press.
   * The start positions are recomputed per receiver as the event bubbles.
   */
  struct DragEvent : MouseButtonEvent
  {
    Point start_global_pos;
    Point start_local_pos;
    Point delta;
    Point total_delta;
    Roo::sptr_val payload = Roo::Constant::NIL;
  };

  /**
   * Keyboard event delivered to the focused view chain, or to the root view
   * when nothing is focused. For key down/up hooks, `key` carries the
   * translated Roo key keyword, such as :key/space or :key/left.
   *
   * For key-held hooks, `held_keys` carries the full set of currently held
   * keys for the frame, and `match` carries the declarative held-key spec
   * that matched when dispatched from an on-key-held map.
   */
  struct KeyboardEvent : Event
  {
    Roo::sptr_val key = Roo::Constant::NIL;
    Roo::sptr_val held_keys = Roo::Constant::NIL;
    Roo::sptr_val match = Roo::Constant::NIL;
  };

} // namespace Pixils

#endif /* PIXILS__UI__EVENT_H */
