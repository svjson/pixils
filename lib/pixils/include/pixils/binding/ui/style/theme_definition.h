#ifndef PIXILS__BINDING__THEME_DEFINITION_H
#define PIXILS__BINDING__THEME_DEFINITION_H

#include <pixils/ui/theme.h>

#include <lisple/runtime/value.h>

#include <optional>
#include <string>

namespace Lisple
{
  class Context;
}

namespace Pixils::Script
{
  Lisple::sptr_val lookup_theme_var(const UI::Theme& theme,
                                    const std::optional<std::string>& variant,
                                    const std::string& key);
  bool contains_theme_var_ref(const Lisple::sptr_val& value);
  Lisple::sptr_val resolve_theme_vars(const UI::Theme& theme,
                                      const std::optional<std::string>& variant,
                                      const Lisple::sptr_val& value,
                                      int depth = 0);
  UI::Theme build_theme_from_definition(Lisple::Context& ctx,
                                        const std::string& name,
                                        const Lisple::sptr_val& definition_map,
                                        const UI::Theme* base = nullptr);
}

#endif
