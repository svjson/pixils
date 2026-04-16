
#include "pixils/ui/style.h"

#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::UI
{
  /** Style */
  Style::Style() {}

  Style::Style(const Style& other)
    : background(other.background)
    , padding(other.padding)
    , hover_style(other.hover_style ? std::make_unique<Style>(*other.hover_style) : nullptr)
  {
  }

  void Style::operator=(const Style& other)
  {
    this->background = other.background;
    this->padding = other.padding;
    this->hover_style =
      other.hover_style ? std::make_unique<Style>(*other.hover_style) : nullptr;
  }

  /** Style::Background */
  Style::Background::Background() {}

  Style::Background::Background(const Color& color)
    : color(color)
  {
  }

  Style::Background::Background(const std::pair<std::string, std::string>& image)
    : image(image)
  {
  }

  /** Style::Padding */
  Style::Padding::Padding(int t, int r, int b, int l)
    : t(t)
    , r(r)
    , b(b)
    , l(l)
  {
  }

  Style::Padding::Padding(int h, int v)
    : t(v)
    , r(h)
    , b(v)
    , l(h)
  {
  }

  Style::Padding::Padding(const Padding& other)
    : t(other.t)
    , r(other.r)
    , b(other.b)
    , l(other.l)
  {
  }

  Rect Style::Padding::apply_to(const Rect& rect) const
  {
    return Rect(rect.x + this->l,
                rect.y + this->t,
                rect.w - this->l - this->r,
                rect.h - this->t - this->b);
  }

  /**
   * Merge properties from a style variant map into a ResolvedStyle.
   * Only non-NIL properties in the variant override existing values.
   */
  void apply_style_variant(Style& out, const Style& variant)
  {
    if (variant.background)
    {
      if (!out.background) out.background = Style::Background();
      if (variant.background->color) out.background->color = *variant.background->color;
      if (variant.background->image) out.background->image = *variant.background->image;
    }

    if (variant.padding)
    {
      out.padding = variant.padding;
    }
  }

  UI::Style resolve_style(const std::optional<Style>& style, const Lisple::sptr_rtval& state)
  {
    UI::Style result;
    if (!style) return result;

    apply_style_variant(result, *style);

    if (state && state->type == Lisple::RTValue::Type::MAP)
    {
      auto hovered = Lisple::Dict::get_property(state, Lisple::RTValue::keyword("hovered"));
      if (hovered && Lisple::is_truthy(*hovered) && style->hover_style)
        apply_style_variant(result, *style->hover_style);

      // auto active = Lisple::Dict::get_property(state, Lisple::RTValue::keyword("active"));
      // if (active && Lisple::is_truthy(*active))
      //   apply_style_variant(result, Lisple::Dict::get_property(*style, "active"));
    }

    return result;
  }

} // namespace Pixils::UI
