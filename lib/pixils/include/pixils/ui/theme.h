#ifndef PIXILS__UI__THEME_H
#define PIXILS__UI__THEME_H

#include <pixils/ui/style.h>

#include <cstdint>
#include <lisple/runtime/value.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Pixils::UI
{
  struct ThemeMatchContext
  {
    std::vector<std::string> mode_names;
    std::vector<std::string> class_names;
    Lisple::sptr_val state = Lisple::Constant::NIL;
    InteractionState interaction;
  };

  struct ThemeSelector
  {
    enum class Type : uint8_t
    {
      COMPONENT_TYPE,
      CLASS_NAME,
      STATE,
      COMPOUND,
      DESCENDANT,
    };

    Type type = Type::COMPONENT_TYPE;
    std::string value;
    Lisple::sptr_val state = Lisple::Constant::NIL;
    std::vector<ThemeSelector> children;
    bool hovered = false;
    bool focused = false;
    bool focus_within = false;
    bool disabled = false;

    static ThemeSelector component_type(const std::string& value);
    static ThemeSelector class_name(const std::string& value);
    static ThemeSelector state_match(const Lisple::sptr_val& value);
    static ThemeSelector compound(const std::vector<ThemeSelector>& children);
    static ThemeSelector descendant(const std::vector<ThemeSelector>& children);

    bool operator==(const ThemeSelector& other) const;
    bool matches(const ThemeMatchContext& ctx) const;
    bool matches_path(const std::vector<ThemeMatchContext>& path) const;
    int specificity() const;
  };

  struct ThemeRule
  {
    ThemeSelector selector;
    Style style;
    std::vector<Lisple::sptr_val> style_exprs;
  };

  struct Theme
  {
    std::string name;
    std::vector<std::string> extend;
    std::optional<std::string> default_variant = std::nullopt;
    std::optional<std::string> selected_variant = std::nullopt;
    bool declarations_resolved = false;
    std::optional<Style> defaults = std::nullopt;
    std::vector<Lisple::sptr_val> defaults_exprs;
    std::map<std::string, std::map<std::string, Lisple::sptr_val>> vars;
    std::vector<ThemeRule> rules;
    std::map<std::string, Style> variant_defaults;
    std::map<std::string, std::vector<Lisple::sptr_val>> variant_defaults_exprs;
    std::map<std::string, std::vector<ThemeRule>> variant_rules;

    void set_style(const ThemeSelector& selector,
                   const Style& style,
                   const std::vector<Lisple::sptr_val>& style_exprs = {});
    void set_variant_style(const std::string& variant,
                           const ThemeSelector& selector,
                           const Style& style,
                           const std::vector<Lisple::sptr_val>& style_exprs = {});
    const Style* get_style(const ThemeSelector& selector) const;
    const Style* get_variant_style(const std::string& variant,
                                   const ThemeSelector& selector) const;
    std::vector<const Style*> get_matching_styles(const ThemeMatchContext& ctx) const;
    std::vector<const Style*> get_matching_styles(
      const std::vector<ThemeMatchContext>& path) const;
    Theme resolved_for_variant(const std::optional<std::string>& variant) const;
  };

  void overlay_theme(Theme& out, const Theme& overlay);
} // namespace Pixils::UI

#endif
