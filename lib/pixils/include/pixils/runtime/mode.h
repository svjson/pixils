#ifndef PIXILS__RUNTIME__MODE_H
#define PIXILS__RUNTIME__MODE_H

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

  struct Mode
  {
    std::string name;
    ResourceDependencies resources;
    Lisple::sptr_sobject init;
    Lisple::sptr_sobject update;
    Lisple::sptr_sobject render;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_H */
