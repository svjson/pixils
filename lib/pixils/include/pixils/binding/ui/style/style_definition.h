#ifndef PIXILS__UI__STYLE__DEFINITION_H
#define PIXILS__UI__STYLE__DEFINITION_H

#include <pixils/ui/style.h>

#include <roo/runtime/value.h>
#include <memory>
#include <optional>

namespace Roo
{
  class Context;
}

namespace Pixils::Script::StyleDefinition
{
  std::optional<UI::Style::Size> parse_size(const Roo::sptr_val& value);
  Roo::sptr_val size_to_value(const std::optional<UI::Style::Size>& size);

  std::optional<UI::Style::Trim> parse_trim(const Roo::sptr_val& value);
  Roo::sptr_val trim_to_value(const std::optional<UI::Style::Trim>& trim);

  std::optional<UI::Style::CornerRadius> parse_corner_radius(Roo::Context& ctx,
                                                             const Roo::sptr_val& value);
  Roo::sptr_val corner_radius_to_value(
    const std::optional<UI::Style::CornerRadius>& radius);

  std::optional<UI::PositionMode> parse_position_mode(const Roo::sptr_val& value);
  Roo::sptr_val position_mode_to_value(const std::optional<UI::PositionMode>& mode);

  std::optional<UI::Style::BoxSizing> parse_box_sizing(const Roo::sptr_val& value);
  Roo::sptr_val box_sizing_to_value(const std::optional<UI::Style::BoxSizing>& value);
  std::optional<UI::Style::Visibility> parse_visibility(const Roo::sptr_val& value);
  Roo::sptr_val visibility_to_value(
    const std::optional<UI::Style::Visibility>& value);
  std::optional<int> parse_scale(const Roo::sptr_val& value);
  Roo::sptr_val scale_to_value(const std::optional<int>& value);
  std::optional<UI::ImageCursor> parse_image_cursor(Roo::Context& ctx,
                                                    const Roo::sptr_val& value);
  std::optional<UI::CursorSpec> parse_cursor(Roo::Context& ctx,
                                             const Roo::sptr_val& value);
  Roo::sptr_val cursor_to_value(const std::optional<UI::CursorSpec>& value);

  std::optional<UI::Style::LineStyle> parse_line_style(const Roo::sptr_val& value);
  Roo::sptr_val line_style_to_value(const std::optional<UI::Style::LineStyle>& value);

  std::optional<Pixils::Text::Alignment> parse_text_align(const Roo::sptr_val& value);
  Roo::sptr_val text_align_to_value(const std::optional<Pixils::Text::Alignment>& value);
  bool parse_text_use_font_color(const Roo::sptr_val& value);
  Roo::sptr_val text_color_to_value(const UI::Style::Text& text);
  std::optional<UI::Style::Text::Wrap> parse_text_wrap(const Roo::sptr_val& value);
  Roo::sptr_val text_wrap_to_value(const std::optional<UI::Style::Text::Wrap>& value);

  std::optional<UI::LayoutDirection> parse_layout_direction(const Roo::sptr_val& value);
  Roo::sptr_val layout_direction_to_value(
    const std::optional<UI::LayoutDirection>& value);
  std::optional<UI::Style::Layout::AlignItems> parse_layout_align_items(
    const Roo::sptr_val& value);
  Roo::sptr_val layout_align_items_to_value(
    const std::optional<UI::Style::Layout::AlignItems>& value);

  std::optional<UI::Style::Layout::GapMode> parse_layout_gap_mode(
    const Roo::sptr_val& value);
  Roo::sptr_val layout_gap_mode_to_value(
    const std::optional<UI::Style::Layout::GapMode>& value);
  std::optional<UI::Style::Layout::Wrap> parse_layout_wrap(
    const Roo::sptr_val& value);
  Roo::sptr_val layout_wrap_to_value(
    const std::optional<UI::Style::Layout::Wrap>& value);

  std::optional<int> parse_optional_int(const Roo::sptr_val& value);
  Roo::sptr_val optional_int_to_value(const std::optional<int>& value);

  std::optional<bool> parse_optional_bool(const Roo::sptr_val& value);
  Roo::sptr_val optional_bool_to_value(const std::optional<bool>& value);

  std::unique_ptr<UI::Style> build_style(Roo::Context& ctx,
                                         const Roo::sptr_val& value);
  std::unique_ptr<UI::Style::Layout> build_layout(Roo::Context& ctx,
                                                  const Roo::sptr_val& value);
  std::unique_ptr<UI::Style::Layout::Gap> build_layout_gap(Roo::Context& ctx,
                                                           const Roo::sptr_val& value);
  std::unique_ptr<UI::Style::Text> build_text(Roo::Context& ctx,
                                              const Roo::sptr_val& value);
  std::unique_ptr<UI::Style::Background> build_background(Roo::Context& ctx,
                                                          const Roo::sptr_val& value);
  std::unique_ptr<UI::Style::Border> build_border(Roo::Context& ctx,
                                                  const Roo::sptr_val& value);
  std::unique_ptr<UI::Style::BorderStyle> build_border_style(Roo::Context& ctx,
                                                             const Roo::sptr_val& value);
  std::unique_ptr<UI::Style::Insets> build_insets(Roo::Context& ctx,
                                                  const Roo::sptr_val& value);
} // namespace Pixils::Script::StyleDefinition

#endif /* PIXILS__UI__STYLE__DEFINITION_H */
