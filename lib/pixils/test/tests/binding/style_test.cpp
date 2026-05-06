
#include "../fixture.h"
#include <pixils/runtime/mode.h>
#include <pixils/ui/style.h>

#include <gtest/gtest.h>
#include <lisple/form.h>
#include <lisple/host/object.h>
#include <lisple/runtime/value.h>
#include <optional>

using StyleTest = BaseFixture;

TEST_F(StyleTest, make_minimal_style)
{
  // When
  Lisple::sptr_rtval result = runtime.eval("(pixils.ui.style/make-style {:width 40})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.width, std::nullopt);
  EXPECT_TRUE(style.width->is_fixed());
  EXPECT_EQ(style.width->fixed_value_or(0), 40);
}

TEST_F(StyleTest, make_uniform_border)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:border {:thickness 1 :line-style :solid}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.border, std::nullopt);
  EXPECT_EQ(style.border->thickness, 1);
  EXPECT_EQ(style.border->line_style, Pixils::UI::Style::LineStyle::SOLID);
}

TEST_F(StyleTest, make_style_with_margin)
{
  // When
  Lisple::sptr_rtval result = runtime.eval("(pixils.ui.style/make-style {:margin [2 4]})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.margin, std::nullopt);
  EXPECT_EQ(style.margin->t, 2);
  EXPECT_EQ(style.margin->r, 4);
  EXPECT_EQ(style.margin->b, 2);
  EXPECT_EQ(style.margin->l, 4);
}

TEST_F(StyleTest, make_style_with_four_value_margin)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:margin [1 2 3 4]})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.margin, std::nullopt);
  EXPECT_EQ(style.margin->t, 1);
  EXPECT_EQ(style.margin->r, 2);
  EXPECT_EQ(style.margin->b, 3);
  EXPECT_EQ(style.margin->l, 4);
}

TEST_F(StyleTest, make_style_with_layout_direction)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:direction :row}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->direction, std::nullopt);
  EXPECT_EQ(*style.layout->direction, Pixils::UI::LayoutDirection::ROW);
}

TEST_F(StyleTest, make_style_with_layout_gap_mode)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:gap {:mode :space-between}}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::SPACE_BETWEEN);
}

TEST_F(StyleTest, make_style_with_layout_gap_keyword)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:gap :space-between}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::SPACE_BETWEEN);
}

TEST_F(StyleTest, make_style_with_layout_gap_none_keyword)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:gap :none}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::NONE);
}

TEST_F(StyleTest, make_style_with_layout_gap_number)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:layout {:gap 8}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.layout, std::nullopt);
  ASSERT_NE(style.layout->gap, std::nullopt);
  ASSERT_NE(style.layout->gap->mode, std::nullopt);
  ASSERT_NE(style.layout->gap->size, std::nullopt);
  EXPECT_EQ(*style.layout->gap->mode, Pixils::UI::Style::Layout::GapMode::FIXED);
  EXPECT_EQ(*style.layout->gap->size, 8);
}

TEST_F(StyleTest, make_style_with_fill_and_shrink_sizes)
{
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:width :fill :height :shrink})");

  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.width, std::nullopt);
  ASSERT_NE(style.height, std::nullopt);
  EXPECT_TRUE(style.width->is_fill());
  EXPECT_TRUE(style.height->is_shrink());
}

TEST_F(StyleTest, make_style_with_text)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:text {:color {:r 255 :g 255 :b 255} "
                 ":font :font/console :scale 2}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.text, std::nullopt);
  ASSERT_NE(style.text->color, std::nullopt);
  ASSERT_NE(style.text->font, std::nullopt);
  ASSERT_NE(style.text->scale, std::nullopt);
  EXPECT_EQ(*style.text->color, (Pixils::Color{255, 255, 255, 255}));
  EXPECT_EQ(*style.text->font, "font/console");
  EXPECT_EQ(*style.text->scale, 2);
}

TEST(StyleResolveTest, inherited_text_fields_are_applied_fieldwise)
{
  Pixils::UI::Style parent;
  parent.text = Pixils::UI::Style::Text{};
  parent.text->color = Pixils::Color{255, 255, 255, 255};
  parent.text->font = "font/ui";
  parent.text->scale = 2;

  Pixils::UI::Style child;
  child.text = Pixils::UI::Style::Text{};
  child.text->color = Pixils::Color{255, 0, 0, 255};

  auto resolved = Pixils::UI::resolve_style(std::optional<Pixils::UI::Style>{child},
                                            &parent,
                                            Lisple::Constant::NIL,
                                            {});

  ASSERT_NE(resolved.text, std::nullopt);
  ASSERT_NE(resolved.text->color, std::nullopt);
  ASSERT_NE(resolved.text->font, std::nullopt);
  ASSERT_NE(resolved.text->scale, std::nullopt);
  EXPECT_EQ(*resolved.text->color, (Pixils::Color{255, 0, 0, 255}));
  EXPECT_EQ(*resolved.text->font, "font/ui");
  EXPECT_EQ(*resolved.text->scale, 2);
}

TEST(StyleResolveTest, hover_text_variant_overrides_only_its_own_fields)
{
  Pixils::UI::Style parent;
  parent.text = Pixils::UI::Style::Text{};
  parent.text->font = "font/ui";
  parent.text->scale = 2;

  Pixils::UI::Style child;
  child.hover = std::make_unique<Pixils::UI::Style>();
  child.hover->text = Pixils::UI::Style::Text{};
  child.hover->text->color = Pixils::Color{0, 255, 0, 255};

  Pixils::UI::InteractionState interaction;
  interaction.hovered = true;

  auto resolved = Pixils::UI::resolve_style(std::optional<Pixils::UI::Style>{child},
                                            &parent,
                                            Lisple::Constant::NIL,
                                            interaction);

  ASSERT_NE(resolved.text, std::nullopt);
  ASSERT_NE(resolved.text->color, std::nullopt);
  ASSERT_NE(resolved.text->font, std::nullopt);
  ASSERT_NE(resolved.text->scale, std::nullopt);
  EXPECT_EQ(*resolved.text->color, (Pixils::Color{0, 255, 0, 255}));
  EXPECT_EQ(*resolved.text->font, "font/ui");
  EXPECT_EQ(*resolved.text->scale, 2);
}

TEST_F(StyleTest, make_insets_with_four_value_vector)
{
  // When
  Lisple::sptr_rtval result = runtime.eval("(pixils.ui.style/make-insets [1 2 3 4])");

  // Then
  auto insets = Lisple::obj<Pixils::UI::Style::Insets>(*result);
  EXPECT_EQ(insets.t, 1);
  EXPECT_EQ(insets.r, 2);
  EXPECT_EQ(insets.b, 3);
  EXPECT_EQ(insets.l, 4);
}

TEST_F(StyleTest, make_bevel_border)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-style {:border {:thickness 2 :line-style :bevel}})");

  // Then
  auto style = Lisple::obj<Pixils::UI::Style>(*result);
  ASSERT_NE(style.border, std::nullopt);
  EXPECT_EQ(style.border->thickness, 2);
  EXPECT_EQ(style.border->line_style, Pixils::UI::Style::LineStyle::BEVEL);
}

TEST(StyleTotalDimensionsTest, total_width_includes_padding_and_border)
{
  // Given
  Pixils::UI::Style style;
  style.width = 100;
  style.padding = Pixils::UI::Style::Insets(4, 4, 4, 4);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;

  // Then: content=100, padding=4+4=8, border=2+2=4 -> total=112
  EXPECT_EQ(style.total_width(), 112);
}

TEST(StyleTotalDimensionsTest, total_width_includes_margin_padding_and_border)
{
  // Given
  Pixils::UI::Style style;
  style.width = 100;
  style.margin = Pixils::UI::Style::Insets(0, 3, 0, 5);
  style.padding = Pixils::UI::Style::Insets(4, 4, 4, 4);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;

  // Then: content=100, margin=5+3=8, padding=4+4=8, border=2+2=4 -> total=120
  EXPECT_EQ(style.total_width(), 120);
}

TEST(StyleTotalDimensionsTest, total_height_includes_padding_and_border)
{
  // Given
  Pixils::UI::Style style;
  style.height = 50;
  style.padding = Pixils::UI::Style::Insets(3, 0, 3, 0);
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 1;

  // Then: content=50, padding=3+3=6, border=1+1=2 -> total=58
  EXPECT_EQ(style.total_height(), 58);
}

TEST(StyleContentRectTest, content_rect_insets_by_border_then_padding)
{
  // Given
  Pixils::UI::Style style;
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 2;
  style.padding = Pixils::UI::Style::Insets(4, 4, 4, 4);

  // When
  Pixils::Rect result = style.content_rect({0, 0, 100, 60});

  // Then: border inset -> {2,2,96,56}, padding inset -> {6,6,88,48}
  EXPECT_EQ(result.x, 6);
  EXPECT_EQ(result.y, 6);
  EXPECT_EQ(result.w, 88);
  EXPECT_EQ(result.h, 48);
}

TEST(StyleContentRectTest, content_rect_with_border_only)
{
  // Given
  Pixils::UI::Style style;
  style.border = Pixils::UI::Style::BorderStyle{};
  style.border->thickness = 3;

  // When
  Pixils::Rect result = style.content_rect({10, 20, 100, 80});

  // Then
  EXPECT_EQ(result.x, 13);
  EXPECT_EQ(result.y, 23);
  EXPECT_EQ(result.w, 94);
  EXPECT_EQ(result.h, 74);
}

TEST(StyleContentRectTest, content_rect_with_no_border_or_padding_returns_bounds)
{
  // Given
  Pixils::UI::Style style;

  // When
  Pixils::Rect result = style.content_rect({5, 10, 200, 100});

  // Then
  EXPECT_EQ(result.x, 5);
  EXPECT_EQ(result.y, 10);
  EXPECT_EQ(result.w, 200);
  EXPECT_EQ(result.h, 100);
}
