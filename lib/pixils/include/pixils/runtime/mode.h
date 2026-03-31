#ifndef PIXILS__RUNTIME__MODE_H
#define PIXILS__RUNTIME__MODE_H

#include "pixils/binding/pixils_namespace.h"

#include <lisple/form.h>
#include <string>

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
  };

  struct Mode
  {
    std::string name;
    ResourceDependencies resources;
    Lisple::sptr_sobject init;
    Lisple::sptr_sobject update;
    Lisple::sptr_sobject render;
    ModeComposition composition;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_H */
