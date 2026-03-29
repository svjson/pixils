
#ifndef __PIXILS_MODE_H_
#define __PIXILS_MODE_H_

#include <lisple/form.h>

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

#endif /* MODE_H */
