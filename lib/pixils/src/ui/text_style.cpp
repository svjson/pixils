#include <pixils/context.h>
#include <pixils/ui/text_style.h>

namespace Pixils::UI::TextStyle
{
  float scale(const Style& style)
  {
    if (style.text && style.text->scale) return *style.text->scale;
    return 1.0f;
  }

  std::optional<Color> color(const Style& style)
  {
    if (style.text && style.text->use_font_color) return std::nullopt;
    if (style.text && style.text->color) return style.text->color;
    return std::nullopt;
  }

  std::string font_key(const Style& style)
  {
    if (style.text && style.text->font) return *style.text->font;
    return "font/console";
  }

  std::vector<Text::FontStyle> font_styles(const Style& style)
  {
    if (style.text && style.text->font_styles) return *style.text->font_styles;
    return {};
  }

  std::vector<Text::Shadow> shadows(const Style& style)
  {
    if (style.text && style.text->shadows) return *style.text->shadows;
    return {};
  }

  std::optional<Text::InlineTextStyleSpec> marked_style(const Style& style)
  {
    if (!style.text || !style.text->marked_style) return std::nullopt;

    Text::InlineTextStyleSpec inline_style;
    inline_style.enabled = style.text->marked_style->enabled.value_or(true);
    inline_style.marker = style.text->marked_style->marker.value_or('@');
    inline_style.use_font_color = style.text->marked_style->use_font_color.value_or(false);
    inline_style.color = style.text->marked_style->color;
    inline_style.font_key = style.text->marked_style->font;
    if (style.text->marked_style->scale)
      inline_style.scale = Text::Scale(*style.text->marked_style->scale);
    inline_style.font_styles = style.text->marked_style->font_styles;
    inline_style.shadows = style.text->marked_style->shadows;
    return inline_style;
  }

  Text::Alignment alignment(const Style& style)
  {
    if (style.text && style.text->align) return *style.text->align;
    return Text::Alignment::LEFT;
  }

  Text::WrapMode wrap_mode(const Style& style)
  {
    if (!style.text || !style.text->wrap) return Text::WrapMode::WORD;
    return *style.text->wrap == Style::Text::Wrap::NONE ? Text::WrapMode::NONE
                                                        : Text::WrapMode::WORD;
  }

  std::optional<Text::TextRenderOp> make_render_op(RenderContext& rc,
                                                   const Style& style,
                                                   const std::optional<Color>& color)
  {
    auto key = font_key(style);
    auto text_op = Text::make_text_render_op(rc,
                                             key,
                                             scale(style),
                                             color,
                                             font_styles(style),
                                             shadows(style),
                                             marked_style(style));
    if (!text_op && key != "font/console")
    {
      text_op = Text::make_text_render_op(rc,
                                          "font/console",
                                          scale(style),
                                          color,
                                          font_styles(style),
                                          shadows(style),
                                          marked_style(style));
    }
    return text_op;
  }
} // namespace Pixils::UI::TextStyle
