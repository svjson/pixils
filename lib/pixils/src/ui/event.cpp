
#include "pixils/ui/event.h"

namespace Pixils
{
  CustomEvent::CustomEvent(const Lisple::sptr_rtval& event_key,
                           const Lisple::sptr_rtval& payload)
    : event_key(event_key)
    , payload(payload)
  {
  }

} // namespace Pixils
