#ifndef PIXILS__UI__THEME_H
#define PIXILS__UI__THEME_H

#include <pixils/ui/style.h>

#include <cstdint>
#include <lisple/runtime/value.h>
#include <string>
#include <vector>

namespace Pixils::UI
{
  struct ThemeMatchContext
  {
    std::vector<std::string> mode_names;
    std::vector<std::string> class_names;
    Lisple::sptr_rtval state = Lisple::Constant::NIL;
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
    Lisple::sptr_rtval state = Lisple::Constant::NIL;
    std::vector<ThemeSelector> children;

    static ThemeSelector component_type(const std::string& value);
    static ThemeSelector class_name(const std::string& value);
    static ThemeSelector state_match(const Lisple::sptr_rtval& value);
    static ThemeSelector compound(const std::vector<ThemeSelector>& children);
    static ThemeSelector descendant(const std::vector<ThemeSelector>& children);

    bool operator==(const ThemeSelector& other) const;
    bool matches(const ThemeMatchContext& ctx) const;
    int specificity() const;
  };

  struct ThemeRule
  {
    ThemeSelector selector;
    Style style;
  };

  struct Theme
  {
    std::string name;
    std::vector<std::string> extend;
    std::vector<ThemeRule> rules;

    void set_style(const ThemeSelector& selector, const Style& style);
    const Style* get_style(const ThemeSelector& selector) const;
    std::vector<const Style*> get_matching_styles(const ThemeMatchContext& ctx) const;
  };

  void overlay_theme(Theme& out, const Theme& overlay);
} // namespace Pixils::UI

#endif
