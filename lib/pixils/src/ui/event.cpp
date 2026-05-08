
#include "pixils/ui/event.h"

namespace Pixils
{
  CustomEvent::CustomEvent(const Lisple::sptr_rtval& event_key,
                           const Lisple::sptr_rtval& payload,
                           const Lisple::sptr_rtval& source_mode)
    : event_key(event_key)
    , source_mode(source_mode)
    , payload(payload)
  {
  }

} // namespace Pixils
