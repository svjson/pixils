#ifndef PIXILS__UI__THEME_H
#define PIXILS__UI__THEME_H

#include <pixils/ui/style.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Pixils::UI
{
  struct ThemeSelector
  {
    enum class Type : uint8_t
    {
      COMPONENT_TYPE,
      CLASS_NAME,
    };

    Type type = Type::COMPONENT_TYPE;
    std::string value;

    static ThemeSelector component_type(const std::string& value);
    static ThemeSelector class_name(const std::string& value);

    bool operator==(const ThemeSelector& other) const;
    bool operator<(const ThemeSelector& other) const;
  };

  struct Theme
  {
    std::string name;
    std::vector<std::string> extend;
    std::map<ThemeSelector, Style> styles;

    void set_style(const ThemeSelector& selector, const Style& style);
    const Style* get_style(const ThemeSelector& selector) const;
  };

  void overlay_theme(Theme& out, const Theme& overlay);
} // namespace Pixils::UI

#endif
