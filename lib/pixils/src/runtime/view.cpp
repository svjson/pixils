
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

  void View::queue_replace_child(const std::string& child_id, ChildSlot child_slot)
  {
    pending_child_replacements.push_back(
      QueuedChildReplacement{child_id, std::move(child_slot)});
  }
} // namespace Pixils::Runtime
