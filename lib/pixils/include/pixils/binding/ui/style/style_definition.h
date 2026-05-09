#ifndef PIXILS__UI__STYLE__DEFINITION_H
#define PIXILS__UI__STYLE__DEFINITION_H

#include <pixils/ui/style.h>

#include <lisple/runtime/value.h>

#include <memory>
#include <optional>

namespace Lisple
{
  class Context;
}

namespace Pixils::Script::StyleDefinition
{
  std::optional<UI::Style::Size> parse_size(const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval size_to_value(const std::optional<UI::Style::Size>& size);

  std::optional<UI::Style::Trim> parse_trim(const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval trim_to_value(const std::optional<UI::Style::Trim>& trim);

  std::optional<UI::PositionMode> parse_position_mode(const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval position_mode_to_value(const std::optional<UI::PositionMode>& mode);

  std::optional<UI::Style::BoxSizing> parse_box_sizing(const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval box_sizing_to_value(const std::optional<UI::Style::BoxSizing>& value);

  std::optional<UI::Style::LineStyle> parse_line_style(const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval line_style_to_value(const std::optional<UI::Style::LineStyle>& value);

  std::optional<UI::LayoutDirection> parse_layout_direction(
    const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval layout_direction_to_value(
    const std::optional<UI::LayoutDirection>& value);

  std::optional<UI::Style::Layout::GapMode> parse_layout_gap_mode(
    const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval layout_gap_mode_to_value(
    const std::optional<UI::Style::Layout::GapMode>& value);

  std::optional<int> parse_optional_int(const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval optional_int_to_value(const std::optional<int>& value);

  std::optional<bool> parse_optional_bool(const Lisple::sptr_rtval& value);
  Lisple::sptr_rtval optional_bool_to_value(const std::optional<bool>& value);

  std::unique_ptr<UI::Style> build_style(Lisple::Context& ctx,
                                         const Lisple::sptr_rtval& value);
  std::unique_ptr<UI::Style::Layout> build_layout(Lisple::Context& ctx,
                                                  const Lisple::sptr_rtval& value);
  std::unique_ptr<UI::Style::Layout::Gap> build_layout_gap(
    Lisple::Context& ctx,
    const Lisple::sptr_rtval& value);
  std::unique_ptr<UI::Style::Text> build_text(Lisple::Context& ctx,
                                              const Lisple::sptr_rtval& value);
  std::unique_ptr<UI::Style::Background> build_background(Lisple::Context& ctx,
                                                           const Lisple::sptr_rtval& value);
  std::unique_ptr<UI::Style::Border> build_border(Lisple::Context& ctx,
                                                  const Lisple::sptr_rtval& value);
  std::unique_ptr<UI::Style::BorderStyle> build_border_style(
    Lisple::Context& ctx,
    const Lisple::sptr_rtval& value);
  std::unique_ptr<UI::Style::Insets> build_insets(Lisple::Context& ctx,
                                                  const Lisple::sptr_rtval& value);
} // namespace Pixils::Script::StyleDefinition

#endif /* PIXILS__UI__STYLE__DEFINITION_H */
