#include "pixils/runtime/component_root.h"

#include <pixils/runtime/component.h>
#include <pixils/runtime/state.h>

namespace Pixils::Runtime
{
  Mode make_component_root_mode(const Component& component)
  {
    Mode root_mode;
    root_mode.name = component.name + "-root";

    ChildSlot child;
    child.mode_name = component.name;
    child.id = component.name;
    child.state_binding = make_state_binding();
    root_mode.children.push_back(std::move(child));

    return root_mode;
  }

} // namespace Pixils::Runtime
