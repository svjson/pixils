
#include "pixils/ui/event.h"

namespace Pixils
{
  CustomEvent::CustomEvent(const Roo::sptr_val& event_key,
                           const Roo::sptr_val& payload,
                           const Roo::sptr_val& source_mode)
    : event_key(event_key)
    , source_mode(source_mode)
    , payload(payload)
  {
  }

} // namespace Pixils
