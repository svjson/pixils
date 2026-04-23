
#include "pixils/runtime/view.h"

#include <pixils/ui/event.h>

namespace Pixils::Runtime
{
  void View::emit_event(const CustomEvent& event)
  {
    this->emitted_events.push_back(event);
  }

  void View::drain_events(std::vector<CustomEvent>& collected)
  {
    for (auto& event : this->emitted_events)
    {
      collected.push_back(event);
    }
    this->emitted_events.clear();
  }
} // namespace Pixils::Runtime
