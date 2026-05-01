
#ifndef PIXILS__RUNTIME__EVENT_ROUTING_H
#define PIXILS__RUNTIME__EVENT_ROUTING_H

#include "pixils/ui/event.h"

#include <lisple/runtime/value.h>

namespace Lisple
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct View;

  /**
   * For each event: if receiver has a matching handler, invoke it and merge
   * the updated receiver state back into parent_state immediately. Unhandled
   * events (no handler, or propagation not stopped) are returned so they can
   * bubble to the next ancestor.
   */
  std::vector<CustomEvent> process_view_events(View& receiver,
                                               Lisple::sptr_rtval* parent_state,
                                               Lisple::sptr_rtval& view_ctx,
                                               std::vector<CustomEvent>& events,
                                               Lisple::Runtime& runtime,
                                               bool* receiver_state_updated = nullptr);
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__EVENT_ROUTING_H */
