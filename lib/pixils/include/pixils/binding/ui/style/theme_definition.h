#ifndef PIXILS__BINDING__THEME_DEFINITION_H
#define PIXILS__BINDING__THEME_DEFINITION_H

#include <pixils/ui/theme.h>

#include <lisple/runtime/value.h>

namespace Lisple
{
  class Context;
}

namespace Pixils::Script
{
  UI::Theme build_theme_from_definition(Lisple::Context& ctx,
                                        const std::string& name,
                                        const Lisple::sptr_rtval& definition_map,
                                        const UI::Theme* base = nullptr);
}

#endif
