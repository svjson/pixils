#include <pixils/ui/theme.h>

namespace Pixils::UI
{
  ThemeSelector ThemeSelector::component_type(const std::string& value)
  {
    return ThemeSelector{.type = Type::COMPONENT_TYPE, .value = value};
  }

  ThemeSelector ThemeSelector::class_name(const std::string& value)
  {
    return ThemeSelector{.type = Type::CLASS_NAME, .value = value};
  }

  bool ThemeSelector::operator==(const ThemeSelector& other) const
  {
    return type == other.type && value == other.value;
  }

  bool ThemeSelector::operator<(const ThemeSelector& other) const
  {
    if (type != other.type) return type < other.type;
    return value < other.value;
  }

  void Theme::set_style(const ThemeSelector& selector, const Style& style)
  {
    auto it = styles.find(selector);
    if (it == styles.end())
    {
      styles[selector] = style;
    }
    else
    {
      apply_style_variant(it->second, style);
    }
  }

  const Style* Theme::get_style(const ThemeSelector& selector) const
  {
    auto it = styles.find(selector);
    if (it == styles.end()) return nullptr;
    return &it->second;
  }

  void overlay_theme(Theme& out, const Theme& overlay)
  {
    for (const auto& [selector, style] : overlay.styles)
    {
      auto it = out.styles.find(selector);
      if (it == out.styles.end())
      {
        out.styles[selector] = style;
      }
      else
      {
        apply_style_variant(it->second, style);
      }
    }
  }
} // namespace Pixils::UI
