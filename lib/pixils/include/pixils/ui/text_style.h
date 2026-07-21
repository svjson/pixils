#ifndef PIXILS__UI__TEXT_STYLE_H
#define PIXILS__UI__TEXT_STYLE_H

#include <pixils/color.h>
#include <pixils/text.h>
#include <pixils/ui/style.h>

#include <optional>
#include <string>
#include <vector>

namespace Pixils
{
  struct RenderContext;
}

namespace Pixils::UI::TextStyle
{
  float scale(const Style& style);
  std::optional<Color> color(const Style& style);
  std::string font_key(const Style& style);
  std::vector<Text::FontStyle> font_styles(const Style& style);
  std::vector<Text::Shadow> shadows(const Style& style);
  std::optional<Text::InlineTextStyleSpec> marked_style(const Style& style);
  Text::Alignment alignment(const Style& style);
  Text::WrapMode wrap_mode(const Style& style);

  std::optional<Text::TextRenderOp> make_render_op(
    RenderContext& rc,
    const Style& style,
    const std::optional<Color>& color = std::nullopt);
} // namespace Pixils::UI::TextStyle

#endif /* PIXILS__UI__TEXT_STYLE_H */
