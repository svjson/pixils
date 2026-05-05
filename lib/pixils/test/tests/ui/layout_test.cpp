#include "../fixture.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/frame_events.h>
#include <pixils/geom.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/style.h>
#include <pixils/ui/view_layout.h>

#include <gtest/gtest.h>

using Pixils::Rect;
using Pixils::Runtime::Mode;
using Pixils::Runtime::View;
using Pixils::UI::LayoutDirection;
using Pixils::UI::PositionMode;
using Pixils::UI::Style;

class LayoutTest : public BaseFixture
{
 protected:
  Pixils::FrameEvents events;
  Pixils::HookContext hook_ctx;
  Lisple::sptr_rtval hook_ctx_val;

  LayoutTest()
    : BaseFixture()
    , hook_ctx{&events, &render_ctx}
    , hook_ctx_val(Pixils::Script::HookContextAdapter::make_ref(hook_ctx))
  {
    render_ctx.buffer_dim = {320, 200};
  }

  std::vector<Rect> layout(const std::vector<std::shared_ptr<View>>& children,
                           const Rect& parent,
                           LayoutDirection direction = LayoutDirection::COLUMN)
  {
    return Pixils::UI::layout_children(children, parent, runtime, hook_ctx_val, direction);
  }
};

static std::shared_ptr<View> make_ctx(std::optional<Style> style = std::nullopt)
{
  auto v = std::make_shared<View>();
  v->owned_mode = std::make_unique<Mode>();
  v->owned_mode->style = std::move(style);
  v->mode = v->owned_mode.get();
  v->state = Lisple::Constant::NIL;
  return v;
}

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

TEST_F(LayoutTest, layout_single_fill_child_takes_full_height)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 200);
}

TEST_F(LayoutTest, layout_fixed_then_fill_child_splits_height_correctly)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(40));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 40);
  EXPECT_EQ(rects[1].h, 160);
}

TEST_F(LayoutTest, layout_two_fill_children_split_height_evenly)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx());
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 100);
  EXPECT_EQ(rects[1].y, 100);
  EXPECT_EQ(rects[1].h, 100);
}

TEST_F(LayoutTest, layout_children_without_width_inherit_full_parent_width)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(30));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  for (const auto& r : rects)
  {
    EXPECT_EQ(r.w, 320);
    EXPECT_EQ(r.x, 0);
  }
}

TEST_F(LayoutTest, layout_column_child_honors_requested_width)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 120;
  s.height = 30;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 120);
  EXPECT_EQ(rects[0].h, 30);
}

TEST_F(LayoutTest, layout_child_without_content_size_fills_available_space)
{
  std::vector<std::shared_ptr<View>> children;

  auto container = make_ctx();
  Style child_style;
  child_style.width = 40;
  child_style.height = 10;
  container->children.push_back(make_ctx(std::move(child_style)));
  children.push_back(container);

  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].w, 320);
  EXPECT_EQ(rects[0].h, 200);
}

TEST_F(LayoutTest, layout_child_with_explicit_height_and_derived_width_preserves_both)
{
  std::vector<std::shared_ptr<View>> children;

  Style container_style;
  container_style.height = 25;
  auto container = make_ctx(std::move(container_style));

  Style child_style;
  child_style.width = 40;
  child_style.height = 10;
  container->children.push_back(make_ctx(std::move(child_style)));
  children.push_back(container);

  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].w, 320);
  EXPECT_EQ(rects[0].h, 25);
}

TEST_F(LayoutTest, layout_row_child_without_content_size_fills_available_space)
{
  std::vector<std::shared_ptr<View>> children;

  auto container = make_ctx();
  Style child_style;
  child_style.width = 40;
  child_style.height = 10;
  container->children.push_back(make_ctx(std::move(child_style)));
  children.push_back(container);

  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].w, 320);
  EXPECT_EQ(rects[0].h, 200);
}

TEST_F(LayoutTest, layout_children_respect_parent_origin)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx());
  Rect parent = {10, 20, 100, 80};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 10);
  EXPECT_EQ(rects[0].y, 20);
  EXPECT_EQ(rects[0].w, 100);
  EXPECT_EQ(rects[0].h, 80);
}

TEST_F(LayoutTest, layout_row_direction_fixed_then_fill_splits_width)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(80));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 80);
  EXPECT_EQ(rects[1].x, 80);
  EXPECT_EQ(rects[1].w, 240);
}

TEST_F(LayoutTest, layout_row_child_honors_requested_height)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 80;
  s.height = 40;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].w, 80);
  EXPECT_EQ(rects[0].h, 40);
}

TEST_F(LayoutTest, layout_absolute_children_excluded_from_flow)
{
  Style abs_style;
  abs_style.position = PositionMode::ABSOLUTE;
  abs_style.width = 50;
  abs_style.height = 30;

  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx(std::move(abs_style)));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].w, 0);
  EXPECT_EQ(rects[0].h, 0);
  EXPECT_EQ(rects[1].h, 200);
}

TEST_F(LayoutTest, layout_column_child_margin_offsets_and_insets_rect)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.height = 40;
  s.margin = Style::Insets(2, 4, 6, 8);
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {10, 20, 100, 80};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 18);
  EXPECT_EQ(rects[0].y, 22);
  EXPECT_EQ(rects[0].w, 88);
  EXPECT_EQ(rects[0].h, 40);
}

TEST_F(LayoutTest, layout_column_margins_consume_flow_space)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.height = 40;
  s.margin = Style::Insets(0, 0, 10, 0);
  children.push_back(make_ctx(std::move(s)));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 50);
  EXPECT_EQ(rects[1].h, 150);
}

TEST_F(LayoutTest, layout_row_child_margin_offsets_and_insets_rect)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 80;
  s.margin = Style::Insets(3, 7, 5, 11);
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {10, 20, 120, 60};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 21);
  EXPECT_EQ(rects[0].y, 23);
  EXPECT_EQ(rects[0].w, 80);
  EXPECT_EQ(rects[0].h, 52);
}
