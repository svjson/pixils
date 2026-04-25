
#include "../fixture.h"
#include "pixils/binding/style_namespace.h"
#include <pixils/runtime/mode.h>
#include <pixils/ui/style.h>

#include <gtest/gtest.h>
#include <lisple/form.h>
#include <lisple/host/object.h>
#include <lisple/runtime/value.h>
#include <optional>

using BorderTest = BaseFixture;
using BorderStyleTest = BaseFixture;

TEST(BorderStyleEffectiveThicknessTest, falls_back_to_base_when_no_per_side_override)
{
  // Given
  Pixils::UI::Style::BorderStyle bs;
  bs.thickness = 3;

  // Then
  EXPECT_EQ(bs.top_thickness(), 3);
  EXPECT_EQ(bs.right_thickness(), 3);
  EXPECT_EQ(bs.bottom_thickness(), 3);
  EXPECT_EQ(bs.left_thickness(), 3);
}

TEST(BorderStyleEffectiveThicknessTest, per_side_overrides_base)
{
  // Given
  Pixils::UI::Style::BorderStyle bs;
  bs.thickness = 1;
  bs.t = Pixils::UI::Style::Border{}; bs.t->thickness = 4;
  bs.r = Pixils::UI::Style::Border{}; bs.r->thickness = 3;
  bs.b = Pixils::UI::Style::Border{}; bs.b->thickness = 2;
  bs.l = Pixils::UI::Style::Border{}; bs.l->thickness = 5;

  // Then
  EXPECT_EQ(bs.top_thickness(), 4);
  EXPECT_EQ(bs.right_thickness(), 3);
  EXPECT_EQ(bs.bottom_thickness(), 2);
  EXPECT_EQ(bs.left_thickness(), 5);
}

TEST(BorderStyleEffectiveThicknessTest, returns_zero_when_neither_side_nor_base_set)
{
  // Given
  Pixils::UI::Style::BorderStyle bs;

  // Then
  EXPECT_EQ(bs.top_thickness(), 0);
}

TEST(BorderStyleEffectiveColorTest, falls_back_to_base_when_no_per_side_color)
{
  // Given
  Pixils::UI::Style::BorderStyle bs;
  bs.color = Pixils::Color{255, 0, 0, 255};

  // Then
  EXPECT_EQ(bs.top_color(), bs.color);
  EXPECT_EQ(bs.right_color(), bs.color);
  EXPECT_EQ(bs.bottom_color(), bs.color);
  EXPECT_EQ(bs.left_color(), bs.color);
}

TEST(BorderStyleEffectiveColorTest, per_side_color_overrides_base)
{
  // Given
  Pixils::UI::Style::BorderStyle bs;
  bs.color = Pixils::Color{255, 0, 0, 255};
  bs.t = Pixils::UI::Style::Border{}; bs.t->color = Pixils::Color{0, 255, 0, 255};

  // Then
  EXPECT_EQ(bs.top_color(), (Pixils::Color{0, 255, 0, 255}));
  EXPECT_EQ(bs.right_color(), (Pixils::Color{255, 0, 0, 255}));
}

TEST(BorderStyleApplyToTest, insets_rect_by_uniform_thickness)
{
  // Given
  Pixils::UI::Style::BorderStyle bs;
  bs.thickness = 2;

  // When
  Pixils::Rect result = bs.apply_to({10, 20, 100, 50});

  // Then
  EXPECT_EQ(result.x, 12);
  EXPECT_EQ(result.y, 22);
  EXPECT_EQ(result.w, 96);
  EXPECT_EQ(result.h, 46);
}

TEST(BorderStyleApplyToTest, insets_rect_by_per_side_thickness)
{
  // Given
  Pixils::UI::Style::BorderStyle bs;
  bs.t = Pixils::UI::Style::Border{}; bs.t->thickness = 1;
  bs.r = Pixils::UI::Style::Border{}; bs.r->thickness = 2;
  bs.b = Pixils::UI::Style::Border{}; bs.b->thickness = 3;
  bs.l = Pixils::UI::Style::Border{}; bs.l->thickness = 4;

  // When
  Pixils::Rect result = bs.apply_to({0, 0, 100, 100});

  // Then
  EXPECT_EQ(result.x, 4);
  EXPECT_EQ(result.y, 1);
  EXPECT_EQ(result.w, 94);
  EXPECT_EQ(result.h, 96);
}

TEST_F(BorderTest, make_border)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-border {:thickness 1 :line-style :solid})");

  // Then
  auto border = Lisple::obj<Pixils::UI::Style::Border>(*result);
  EXPECT_EQ(border.line_style, Pixils::UI::Style::LineStyle::SOLID);
  EXPECT_EQ(border.thickness, 1);
  EXPECT_EQ(border.color, std::nullopt);
}

TEST_F(BorderStyleTest, make_border_style)
{
  // When
  Lisple::sptr_rtval result =
    runtime.eval("(pixils.ui.style/make-border-style {:thickness 1 :line-style :solid})");

  // Then
  ASSERT_TRUE(Pixils::Script::HostType::BORDER_STYLE.is_type_of(*result));
  auto& border = result->adapter<Pixils::Script::BorderStyleAdapter>().get_self_object();
  EXPECT_EQ(border.line_style, Pixils::UI::Style::LineStyle::SOLID);
  EXPECT_EQ(border.thickness, 1);
  EXPECT_EQ(border.color, std::nullopt);
}
