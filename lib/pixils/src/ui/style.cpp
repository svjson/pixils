
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
    , width(other.width)
    , height(other.height)
    , position(other.position)
    , top(other.top)
    , left(other.left)
    , direction(other.direction)
    , hidden(other.hidden)
    , hover(other.hover ? std::make_unique<Style>(*other.hover) : nullptr)
  {
  }

  void Style::operator=(const Style& other)
  {
    this->background = other.background;
    this->padding = other.padding;
    this->width = other.width;
    this->height = other.height;
    this->position = other.position;
    this->top = other.top;
    this->left = other.left;
    this->direction = other.direction;
    this->hidden = other.hidden;
    this->hover = other.hover ? std::make_unique<Style>(*other.hover) : nullptr;
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

  /** Style::Insets */
  Style::Insets::Insets(int t, int r, int b, int l)
    : t(t)
    , r(r)
    , b(b)
    , l(l)
  {
  }

  Style::Insets::Insets(int h, int v)
    : t(v)
    , r(h)
    , b(v)
    , l(h)
  {
  }

  Style::Insets::Insets(const Insets& other)
    : t(other.t)
    , r(other.r)
    , b(other.b)
    , l(other.l)
  {
  }

  Rect Style::Insets::apply_to(const Rect& rect) const
  {
    return Rect(rect.x + this->l,
                rect.y + this->t,
                rect.w - this->l - this->r,
                rect.h - this->t - this->b);
  }

  int Style::total_width() const
  {
    int pad = padding ? padding->l + padding->r : 0;
    return width.value_or(0) + pad;
  }

  int Style::total_height() const
  {
    int pad = padding ? padding->t + padding->b : 0;
    return height.value_or(0) + pad;
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
    if (variant.padding) out.padding = variant.padding;
    if (variant.width) out.width = variant.width;
    if (variant.height) out.height = variant.height;
    if (variant.position) out.position = variant.position;
    if (variant.top) out.top = variant.top;
    if (variant.left) out.left = variant.left;
    if (variant.direction) out.direction = variant.direction;
    if (variant.hidden) out.hidden = variant.hidden;
  }

  UI::Style resolve_style(const std::optional<Style>& style,
                          const Lisple::sptr_rtval& /* state */,
                          const InteractionState& interaction)
  {
    UI::Style result;
    if (!style) return result;

    apply_style_variant(result, *style);

    if (interaction.hovered && style->hover) apply_style_variant(result, *style->hover);

    return result;
  }

} // namespace Pixils::UI
