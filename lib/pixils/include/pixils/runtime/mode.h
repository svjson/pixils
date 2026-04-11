#ifndef PIXILS__RUNTIME__MODE_H
#define PIXILS__RUNTIME__MODE_H

#include "pixils/binding/pixils_namespace.h"

#include <lisple/runtime/value.h>
#include <optional>
#include <string>
#include <vector>

namespace Pixils::Runtime
{
  struct ImageDependency
  {
    std::string resource_id;
    std::string file_name;
  };

  struct ResourceDependencies
  {
    std::vector<ImageDependency> images;
  };

  struct ModeComposition
  {
    bool render = false;
    bool update = false;
  };

  struct Mode
  {
    std::string name;
    ResourceDependencies resources;
    Lisple::sptr_rtval init;
    Lisple::sptr_rtval update;
    Lisple::sptr_rtval render;
    ModeComposition composition;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_H */
