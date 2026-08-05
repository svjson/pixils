#ifndef PIXILS__RUNTIME__COMPONENT_ROOT_H
#define PIXILS__RUNTIME__COMPONENT_ROOT_H

#include <pixils/runtime/mode.h>

namespace Pixils::Runtime
{
  struct Component;

  /**
   * Build the generated root mode used when a public root entry point receives
   * a component symbol. The component remains a child view; the mode stack
   * still stores only the returned mode.
   */
  Mode make_component_root_mode(const Component& component);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__COMPONENT_ROOT_H */
