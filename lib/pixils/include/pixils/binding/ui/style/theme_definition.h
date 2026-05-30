#ifndef PIXILS__BINDING__THEME_DEFINITION_H
#define PIXILS__BINDING__THEME_DEFINITION_H

#include <pixils/ui/theme.h>

#include <roo/runtime/value.h>
#include <optional>
#include <string>

namespace Roo
{
  class Context;
}

namespace Pixils::Script
{
  Roo::sptr_val lookup_theme_var(const UI::Theme& theme,
                                    const std::optional<std::string>& variant,
                                    const std::string& key);
  bool contains_theme_var_ref(const Roo::sptr_val& value);
  Roo::sptr_val resolve_theme_vars(const UI::Theme& theme,
                                      const std::optional<std::string>& variant,
                                      const Roo::sptr_val& value,
                                      int depth = 0);
  UI::Theme resolve_theme_declarations(Roo::Context& ctx,
                                       const UI::Theme& theme,
                                       const std::optional<std::string>& variant);
  UI::Theme build_theme_from_definition(Roo::Context& ctx,
                                        const std::string& name,
                                        const Roo::sptr_val& definition_map,
                                        const UI::Theme* base = nullptr);
} // namespace Pixils::Script

#endif
