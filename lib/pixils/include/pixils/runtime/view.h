
#ifndef PIXILS__RUNTIME__VIEW_H
#define PIXILS__RUNTIME__VIEW_H

#include "pixils/ui/event.h"
#include <pixils/runtime/mode.h>
#include <pixils/ui/interaction.h>

#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  /**
   * Live instance of a mode. Serves as the runtime companion for any mode -
   * whether active at the top of the mode stack, participating in composition
   * below it, or placed as a layout child of another mode. Holds the resolved
   * mode pointer, Lisple state, last-computed layout bounds, and any nested
   * child views. For layout children, `id` is the key under which this
   * view's state is stored in the parent state map.
   */
  struct View
  {
    std::string id;
    Lisple::sptr_rtval state_binding = Lisple::Constant::NIL;
    Mode* mode = nullptr;
    UI::InteractionState interaction;
    /**
     * Owns a per-instance copy of the mode when push-time or
     * child-slot overrides are present. `mode` points here instead of
     * into the shared registry. unique_ptr so that the pointer
     * remains valid when View is moved.
     */
    std::unique_ptr<Mode> owned_mode;
    Lisple::sptr_rtval state = Lisple::Constant::NIL;
    Lisple::sptr_rtval initial_state = Lisple::Constant::NIL;
    Rect bounds = {0, 0, 0, 0};
    UI::Style effective_style;
    std::vector<std::shared_ptr<View>> children;
    std::vector<CustomEvent> emitted_events;

    void emit_event(const CustomEvent& event);
    void drain_events(std::vector<CustomEvent>& collected);
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__VIEW_H */
