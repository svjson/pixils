#ifndef PIXILS__UI__VIEW_EVENTS_H
#define PIXILS__UI__VIEW_EVENTS_H

#include "pixils/ui/event.h"

#include <lisple/runtime/value.h>

namespace Lisple
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct View;
}

namespace Pixils::UI
{
  /**
   * For each event: if receiver has a matching handler, invoke it and merge
   * the updated receiver state back into parent_state immediately. Unhandled
   * events (no handler, or propagation not stopped) are returned so they can
   * bubble to the next ancestor.
   */
  std::vector<CustomEvent> process_view_events(Runtime::View& receiver,
                                               Lisple::sptr_val* parent_state,
                                               Runtime::View* parent_view,
                                               Lisple::sptr_val& view_ctx,
                                               std::vector<CustomEvent>& events,
                                               Lisple::Runtime& runtime,
                                               bool* receiver_state_updated = nullptr);
} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_EVENTS_H */
