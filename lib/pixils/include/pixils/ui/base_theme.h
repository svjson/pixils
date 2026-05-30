#ifndef PIXILS__UI__BASE_THEME_H
#define PIXILS__UI__BASE_THEME_H

#include <pixils/ui/theme.h>

namespace Roo
{
  class Runtime;
}

namespace Pixils::UI
{
  const Theme& default_base_theme(Roo::Runtime& runtime);
}

#endif
