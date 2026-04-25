
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
  EXPECT_EQ(style.width, 40);
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
