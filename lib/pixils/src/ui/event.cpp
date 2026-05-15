
#include "pixils/ui/event.h"

namespace Pixils
{
  CustomEvent::CustomEvent(const Lisple::sptr_val& event_key,
                           const Lisple::sptr_val& payload,
                           const Lisple::sptr_val& source_mode)
    : event_key(event_key)
    , source_mode(source_mode)
    , payload(payload)
  {
  }

} // namespace Pixils
