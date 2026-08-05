#ifndef PIXILS__RUNTIME__MODE_H
#define PIXILS__RUNTIME__MODE_H

#include <pixils/runtime/view_definition.h>

namespace Pixils::Runtime
{
  struct ModeComposition
  {
    bool render = false;
    bool update = false;
    bool interaction_pass = false;
    bool interaction_refresh = false;
  };

  struct Mode : ViewDefinition
  {
    ModeComposition composition;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_H */
