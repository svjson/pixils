
#include <pixils/geom.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/style.h>
#include <pixils/ui/view_layout.h>
#include <pixils/ui/view_render.h>

#include <gtest/gtest.h>

using Pixils::Rect;
using Pixils::Runtime::Mode;
using Pixils::Runtime::View;
using Pixils::UI::LayoutDirection;
using Pixils::UI::PositionMode;
using Pixils::UI::Style;

/**
 * Build a View whose mode has the given style. The Mode is owned by the view.
 */
static std::shared_ptr<View> make_ctx(std::optional<Style> style = std::nullopt)
{
  auto v = std::make_shared<View>();
  v->owned_mode = std::make_unique<Mode>();
  v->owned_mode->style = std::move(style);
  v->mode = v->owned_mode.get();
  v->state = Lisple::Constant::NIL;
  return v;
}

/**
 * Build a View whose mode has a fixed size on one axis.
 */
static std::shared_ptr<View> make_fixed_ctx(int height)
{
  Style s;
  s.height = height;
  return make_ctx(std::move(s));
}

static std::shared_ptr<View> make_fixed_width_ctx(int width)
{
  Style s;
  s.width = width;
  return make_ctx(std::move(s));
}

TEST(LayoutTest, layout_single_fill_child_takes_full_height)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 200);
}

TEST(LayoutTest, layout_fixed_then_fill_child_splits_height_correctly)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(40));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 40);
  EXPECT_EQ(rects[1].h, 160);
}

TEST(LayoutTest, layout_two_fill_children_split_height_evenly)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx());
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 100);
  EXPECT_EQ(rects[1].y, 100);
  EXPECT_EQ(rects[1].h, 100);
}

TEST(LayoutTest, layout_children_without_width_inherit_full_parent_width)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(30));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  for (const auto& r : rects)
  {
    EXPECT_EQ(r.w, 320);
    EXPECT_EQ(r.x, 0);
  }
}

TEST(LayoutTest, layout_column_child_honors_requested_width)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 120;
  s.height = 30;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 120);
  EXPECT_EQ(rects[0].h, 30);
}

TEST(LayoutTest, layout_children_respect_parent_origin)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx());
  Rect parent = {10, 20, 100, 80};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 10);
  EXPECT_EQ(rects[0].y, 20);
  EXPECT_EQ(rects[0].w, 100);
  EXPECT_EQ(rects[0].h, 80);
}

TEST(LayoutTest, layout_row_direction_fixed_then_fill_splits_width)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(80));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent, LayoutDirection::ROW);

  // Then
  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 80);
  EXPECT_EQ(rects[1].x, 80);
  EXPECT_EQ(rects[1].w, 240);
}

TEST(LayoutTest, layout_row_child_honors_requested_height)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 80;
  s.height = 40;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent, LayoutDirection::ROW);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].w, 80);
  EXPECT_EQ(rects[0].h, 40);
}

TEST(LayoutTest, layout_absolute_children_excluded_from_flow)
{
  // Given
  Style abs_style;
  abs_style.position = PositionMode::ABSOLUTE;
  abs_style.width = 50;
  abs_style.height = 30;

  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx(std::move(abs_style)));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then - absolute child gets zero rect; fill child takes full height
  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].w, 0);
  EXPECT_EQ(rects[0].h, 0);
  EXPECT_EQ(rects[1].h, 200);
}

TEST(LayoutTest, layout_column_child_margin_offsets_and_insets_rect)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.height = 40;
  s.margin = Style::Insets(2, 4, 6, 8);
  auto child = make_ctx(std::move(s));
  children.push_back(child);
  Rect parent = {10, 20, 100, 80};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 18);
  EXPECT_EQ(rects[0].y, 22);
  EXPECT_EQ(rects[0].w, 88);
  EXPECT_EQ(rects[0].h, 40);
}

TEST(LayoutTest, layout_column_margins_consume_flow_space)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.height = 40;
  s.margin = Style::Insets(0, 0, 10, 0);
  auto fixed = make_ctx(std::move(s));
  children.push_back(fixed);
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  // When
  auto rects = Pixils::UI::layout_children(children, parent);

  // Then
  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 50);
  EXPECT_EQ(rects[1].h, 150);
}

TEST(LayoutTest, layout_row_child_margin_offsets_and_insets_rect)
{
  // Given
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 80;
  s.margin = Style::Insets(3, 7, 5, 11);
  auto child = make_ctx(std::move(s));
  children.push_back(child);
  Rect parent = {10, 20, 120, 60};

  // When
  auto rects = Pixils::UI::layout_children(children, parent, LayoutDirection::ROW);

  // Then
  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 21);
  EXPECT_EQ(rects[0].y, 23);
  EXPECT_EQ(rects[0].w, 80);
  EXPECT_EQ(rects[0].h, 52);
}
