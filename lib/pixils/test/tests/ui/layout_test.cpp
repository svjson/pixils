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
  Roo::sptr_val hook_ctx_val;

  LayoutTest()
    : BaseFixture()
    , hook_ctx{&events, &render_ctx}
    , hook_ctx_val(Pixils::Script::HookContextAdapter::make_ref(hook_ctx))
  {
    render_ctx.buffer_dim = {320, 200};
  }

  std::vector<Rect> layout(
    const std::vector<std::shared_ptr<View>>& children,
    const Rect& parent,
    LayoutDirection direction = LayoutDirection::COLUMN,
    std::optional<Style::Layout::GapMode> gap_mode = std::nullopt,
    std::optional<int> gap_size = std::nullopt,
    std::optional<Style::Layout::AlignItems> align_items = std::nullopt,
    std::optional<Style::Layout::Wrap> wrap = std::nullopt,
    std::optional<int> line_gap = std::nullopt)
  {
    Style::Layout layout;
    layout.direction = direction;
    layout.align_items = align_items;
    layout.wrap = wrap;
    layout.line_gap = line_gap;
    if (gap_mode)
    {
      layout.gap = Style::Layout::Gap{};
      layout.gap->mode = *gap_mode;
      layout.gap->size = gap_size;
    }

    return Pixils::UI::layout_children(children, parent, runtime, hook_ctx_val, layout);
  }
};

static std::shared_ptr<View> make_ctx(std::optional<Style> style = std::nullopt)
{
  auto v = std::make_shared<View>();
  v->owned_mode = std::make_unique<Mode>();
  v->owned_mode->style = std::move(style);
  v->mode = v->owned_mode.get();
  v->state = Roo::Constant::NIL;
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

static std::shared_ptr<View> make_hidden_fixed_ctx(int height)
{
  Style s;
  s.height = height;
  s.visibility = Style::Visibility::NONE;
  return make_ctx(std::move(s));
}

static std::shared_ptr<View> make_hidden_fixed_width_ctx(int width)
{
  Style s;
  s.width = width;
  s.visibility = Style::Visibility::NONE;
  return make_ctx(std::move(s));
}

static std::shared_ptr<View> make_invisible_fixed_ctx(int height)
{
  Style s;
  s.height = height;
  s.visibility = Style::Visibility::HIDDEN;
  return make_ctx(std::move(s));
}

static std::shared_ptr<View> make_fill_height_ctx()
{
  Style s;
  s.height = Style::Size(Style::Size::Mode::FILL);
  return make_ctx(std::move(s));
}

static std::shared_ptr<View> make_fill_width_ctx()
{
  Style s;
  s.width = Style::Size(Style::Size::Mode::FILL);
  return make_ctx(std::move(s));
}

static std::shared_ptr<View> make_fill_width_height_ctx()
{
  Style s;
  s.width = Style::Size(Style::Size::Mode::FILL);
  s.height = Style::Size(Style::Size::Mode::FILL);
  return make_ctx(std::move(s));
}

static std::shared_ptr<View> make_shrink_height_ctx(int natural_height)
{
  Style s;
  s.height = Style::Size(Style::Size::Mode::SHRINK);
  auto container = make_ctx(std::move(s));

  Style child_style;
  child_style.height = natural_height;
  container->children.push_back(make_ctx(std::move(child_style)));
  return container;
}

TEST_F(LayoutTest, layout_single_fill_child_takes_full_height)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fill_height_ctx());
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
  children.push_back(make_fill_height_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 40);
  EXPECT_EQ(rects[1].h, 160);
}

TEST_F(LayoutTest, layout_scaled_fixed_child_keeps_logical_size_but_consumes_scaled_space)
{
  Style scaled_style;
  scaled_style.height = 40;
  scaled_style.scale = 2;

  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx(std::move(scaled_style)));
  children.push_back(make_fixed_ctx(20));
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 80);
  EXPECT_EQ(rects[1].h, 20);
}

TEST_F(LayoutTest, layout_scaled_fill_child_gets_inverse_logical_size)
{
  Style scaled_style;
  scaled_style.width = Style::Size(Style::Size::Mode::FILL);
  scaled_style.scale = 2;

  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx(std::move(scaled_style)));
  Rect parent = {0, 0, 200, 40};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 100);
}

TEST_F(LayoutTest, layout_scaled_root_fill_gets_inverse_logical_size)
{
  Style root_style;
  root_style.scale = 2;

  Style child_style;
  child_style.width = Style::Size(Style::Size::Mode::FILL);
  child_style.height = Style::Size(Style::Size::Mode::FILL);

  auto root = make_ctx(std::move(root_style));
  root->children.push_back(make_ctx(std::move(child_style)));

  Pixils::UI::layout_view_tree(root, {0, 0, 320, 200}, runtime, hook_ctx_val);

  EXPECT_EQ(root->bounds.w, 160);
  EXPECT_EQ(root->bounds.h, 100);
  EXPECT_EQ(root->external_bounds.w, 320);
  EXPECT_EQ(root->external_bounds.h, 200);
  ASSERT_EQ(root->children.size(), 1u);
  EXPECT_EQ(root->children[0]->bounds.w, 160);
  EXPECT_EQ(root->children[0]->bounds.h, 100);
}

TEST_F(LayoutTest, layout_shrink_height_child_gives_up_space_when_column_overflows)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(70));
  children.push_back(make_shrink_height_ctx(60));
  children.push_back(make_fixed_ctx(70));
  Rect parent = {0, 0, 320, 150};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 70);
  EXPECT_EQ(rects[1].y, 70);
  EXPECT_EQ(rects[1].h, 10);
  EXPECT_EQ(rects[2].y, 80);
  EXPECT_EQ(rects[2].h, 70);
}

TEST_F(LayoutTest, layout_two_fill_children_split_height_evenly)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fill_height_ctx());
  children.push_back(make_fill_height_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 100);
  EXPECT_EQ(rects[1].y, 100);
  EXPECT_EQ(rects[1].h, 100);
}

TEST_F(LayoutTest, layout_column_fill_child_respects_max_height_and_releases_space)
{
  std::vector<std::shared_ptr<View>> children;

  Style capped;
  capped.height = Style::Size(Style::Size::Mode::FILL);
  capped.max_height = 60;
  children.push_back(make_ctx(std::move(capped)));
  children.push_back(make_fill_height_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 60);
  EXPECT_EQ(rects[1].y, 60);
  EXPECT_EQ(rects[1].h, 140);
}

TEST_F(LayoutTest, layout_children_without_width_do_not_inherit_full_parent_width_by_default)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(30));
  children.push_back(make_ctx());
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children, parent);

  for (const auto& r : rects)
  {
    EXPECT_EQ(r.w, 0);
    EXPECT_EQ(r.x, 0);
  }
}

TEST_F(LayoutTest, layout_children_with_fill_width_inherit_full_parent_width)
{
  std::vector<std::shared_ptr<View>> children;
  Style fixed;
  fixed.height = 30;
  fixed.width = Style::Size(Style::Size::Mode::FILL);
  children.push_back(make_ctx(std::move(fixed)));
  children.push_back(make_fill_width_ctx());
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

TEST_F(LayoutTest, layout_column_align_items_center_centers_fixed_width_child)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 120;
  s.height = 30;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children,
                      parent,
                      LayoutDirection::COLUMN,
                      std::nullopt,
                      std::nullopt,
                      Style::Layout::AlignItems::CENTER);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 100);
  EXPECT_EQ(rects[0].w, 120);
  EXPECT_EQ(rects[0].h, 30);
}

TEST_F(LayoutTest, layout_child_without_content_size_uses_child_tree_natural_main_axis_size)
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
  EXPECT_EQ(rects[0].w, 40);
  EXPECT_EQ(rects[0].h, 10);
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
  EXPECT_EQ(rects[0].w, 40);
  EXPECT_EQ(rects[0].h, 25);
}

TEST_F(LayoutTest,
       layout_row_child_without_content_size_uses_child_tree_natural_main_axis_size)
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
  EXPECT_EQ(rects[0].w, 40);
  EXPECT_EQ(rects[0].h, 10);
}

TEST_F(LayoutTest, layout_children_respect_parent_origin)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fill_width_height_ctx());
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
  children.push_back(make_fill_width_ctx());
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

TEST_F(LayoutTest, layout_row_align_items_center_centers_fixed_height_child)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 80;
  s.height = 40;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 320, 200};

  auto rects = layout(children,
                      parent,
                      LayoutDirection::ROW,
                      std::nullopt,
                      std::nullopt,
                      Style::Layout::AlignItems::CENTER);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].y, 80);
  EXPECT_EQ(rects[0].w, 80);
  EXPECT_EQ(rects[0].h, 40);
}

TEST_F(LayoutTest, layout_fixed_size_defaults_to_border_box)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = 20;
  s.height = 20;
  s.padding = Style::Insets(1, 1, 1, 1);
  s.border = Style::BorderStyle{};
  s.border->thickness = 2;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 100, 100};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].w, 20);
  EXPECT_EQ(rects[0].h, 20);
}

TEST_F(LayoutTest, layout_fixed_content_box_size_adds_padding_and_border)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.box_sizing = Style::BoxSizing::CONTENT_BOX;
  s.width = 20;
  s.height = 20;
  s.padding = Style::Insets(1, 1, 1, 1);
  s.border = Style::BorderStyle{};
  s.border->thickness = 2;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 100, 100};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].w, 26);
  EXPECT_EQ(rects[0].h, 26);
}

TEST_F(LayoutTest, layout_absolute_children_excluded_from_flow)
{
  Style abs_style;
  abs_style.position = PositionMode::ABSOLUTE;
  abs_style.width = 50;
  abs_style.height = 30;

  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_ctx(std::move(abs_style)));
  children.push_back(make_fill_height_ctx());
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
  s.width = Style::Size(Style::Size::Mode::FILL);
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
  children.push_back(make_fill_height_ctx());
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
  s.height = Style::Size(Style::Size::Mode::FILL);
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

TEST_F(LayoutTest, layout_row_space_between_distributes_leftover_across_gaps)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(40));
  children.push_back(make_fixed_width_ctx(40));
  children.push_back(make_fixed_width_ctx(40));
  Rect parent = {0, 0, 200, 40};

  auto rects =
    layout(children, parent, LayoutDirection::ROW, Style::Layout::GapMode::SPACE_BETWEEN);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[1].x, 80);
  EXPECT_EQ(rects[2].x, 160);
}

TEST_F(LayoutTest, layout_column_space_between_distributes_leftover_across_gaps)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(20));
  children.push_back(make_fixed_ctx(20));
  children.push_back(make_fixed_ctx(20));
  Rect parent = {0, 0, 100, 100};

  auto rects =
    layout(children, parent, LayoutDirection::COLUMN, Style::Layout::GapMode::SPACE_BETWEEN);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[1].y, 40);
  EXPECT_EQ(rects[2].y, 80);
}

TEST_F(LayoutTest, layout_space_between_ignores_absolute_children_when_counting_gaps)
{
  Style abs_style;
  abs_style.position = PositionMode::ABSOLUTE;
  abs_style.width = 20;
  abs_style.height = 20;

  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(40));
  children.push_back(make_ctx(std::move(abs_style)));
  children.push_back(make_fixed_width_ctx(40));
  Rect parent = {0, 0, 200, 40};

  auto rects =
    layout(children, parent, LayoutDirection::ROW, Style::Layout::GapMode::SPACE_BETWEEN);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[1].w, 0);
  EXPECT_EQ(rects[2].x, 160);
}

TEST_F(LayoutTest, layout_fixed_gap_ignores_hidden_children)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(20));
  children.push_back(make_hidden_fixed_ctx(80));
  children.push_back(make_fixed_ctx(20));
  Rect parent = {0, 0, 100, 200};

  auto rects =
    layout(children, parent, LayoutDirection::COLUMN, Style::Layout::GapMode::FIXED, 10);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 20);
  EXPECT_EQ(rects[1].h, 0);
  EXPECT_EQ(rects[2].y, 30);
  EXPECT_EQ(rects[2].h, 20);
}

TEST_F(LayoutTest, layout_space_between_ignores_hidden_children_when_counting_gaps)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(40));
  children.push_back(make_hidden_fixed_width_ctx(40));
  children.push_back(make_fixed_width_ctx(40));
  Rect parent = {0, 0, 200, 40};

  auto rects =
    layout(children, parent, LayoutDirection::ROW, Style::Layout::GapMode::SPACE_BETWEEN);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[1].w, 0);
  EXPECT_EQ(rects[2].x, 160);
}

TEST_F(LayoutTest, layout_preserves_space_for_visibility_hidden_children)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(20));
  children.push_back(make_invisible_fixed_ctx(80));
  children.push_back(make_fixed_ctx(20));
  Rect parent = {0, 0, 100, 200};

  auto rects =
    layout(children, parent, LayoutDirection::COLUMN, Style::Layout::GapMode::FIXED, 10);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 20);
  EXPECT_EQ(rects[1].y, 30);
  EXPECT_EQ(rects[1].h, 80);
  EXPECT_EQ(rects[2].y, 120);
  EXPECT_EQ(rects[2].h, 20);
}

TEST_F(LayoutTest, layout_space_between_noops_with_single_flow_child)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(40));
  Rect parent = {0, 0, 200, 40};

  auto rects =
    layout(children, parent, LayoutDirection::ROW, Style::Layout::GapMode::SPACE_BETWEEN);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 40);
}

TEST_F(LayoutTest, layout_row_fixed_gap_uses_requested_spacing)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(40));
  children.push_back(make_fixed_width_ctx(40));
  children.push_back(make_fixed_width_ctx(40));
  Rect parent = {0, 0, 200, 40};

  auto rects =
    layout(children, parent, LayoutDirection::ROW, Style::Layout::GapMode::FIXED, 10);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[1].x, 50);
  EXPECT_EQ(rects[2].x, 100);
}

TEST_F(LayoutTest, layout_column_fixed_gap_reduces_fill_space)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_ctx(40));
  children.push_back(make_fill_height_ctx());
  Rect parent = {0, 0, 100, 200};

  auto rects =
    layout(children, parent, LayoutDirection::COLUMN, Style::Layout::GapMode::FIXED, 10);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[0].h, 40);
  EXPECT_EQ(rects[1].y, 50);
  EXPECT_EQ(rects[1].h, 150);
}

TEST_F(LayoutTest, layout_gap_none_matches_default_behavior)
{
  std::vector<std::shared_ptr<View>> children;
  children.push_back(make_fixed_width_ctx(40));
  children.push_back(make_fixed_width_ctx(40));
  Rect parent = {0, 0, 200, 40};

  auto rects = layout(children, parent, LayoutDirection::ROW, Style::Layout::GapMode::NONE);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[1].x, 40);
}

TEST_F(LayoutTest, layout_row_fill_child_respects_min_width)
{
  std::vector<std::shared_ptr<View>> children;
  Style s;
  s.width = Style::Size(Style::Size::Mode::FILL);
  s.min_width = 120;
  s.height = 20;
  children.push_back(make_ctx(std::move(s)));
  Rect parent = {0, 0, 100, 40};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 1u);
  EXPECT_EQ(rects[0].w, 120);
}

TEST_F(LayoutTest, layout_row_fill_child_respects_max_width_and_releases_space)
{
  std::vector<std::shared_ptr<View>> children;

  Style capped;
  capped.width = Style::Size(Style::Size::Mode::FILL);
  capped.height = 20;
  capped.max_width = 40;
  children.push_back(make_ctx(std::move(capped)));
  children.push_back(make_fill_width_ctx());
  Rect parent = {0, 0, 100, 40};

  auto rects = layout(children, parent, LayoutDirection::ROW);

  ASSERT_EQ(rects.size(), 2u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 40);
  EXPECT_EQ(rects[1].x, 40);
  EXPECT_EQ(rects[1].w, 60);
}

TEST_F(LayoutTest, layout_row_wraps_fixed_children_to_next_line)
{
  std::vector<std::shared_ptr<View>> children;
  for (int i = 0; i < 3; i++)
  {
    Style s;
    s.width = 40;
    s.height = 12;
    children.push_back(make_ctx(std::move(s)));
  }
  Rect parent = {0, 0, 100, 80};

  auto rects = layout(children,
                      parent,
                      LayoutDirection::ROW,
                      Style::Layout::GapMode::FIXED,
                      10,
                      std::nullopt,
                      Style::Layout::Wrap::LINE,
                      5);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].y, 0);
  EXPECT_EQ(rects[1].x, 50);
  EXPECT_EQ(rects[1].y, 0);
  EXPECT_EQ(rects[2].x, 0);
  EXPECT_EQ(rects[2].y, 17);
}

TEST_F(LayoutTest, layout_row_wrap_distributes_fill_children_per_line)
{
  std::vector<std::shared_ptr<View>> children;
  for (int i = 0; i < 3; i++)
  {
    Style s;
    s.width = Style::Size(Style::Size::Mode::FILL);
    s.min_width = 80;
    s.height = 10;
    children.push_back(make_ctx(std::move(s)));
  }
  Rect parent = {0, 0, 200, 80};

  auto rects = layout(children,
                      parent,
                      LayoutDirection::ROW,
                      Style::Layout::GapMode::FIXED,
                      8,
                      std::nullopt,
                      Style::Layout::Wrap::LINE,
                      8);

  ASSERT_EQ(rects.size(), 3u);
  EXPECT_EQ(rects[0].x, 0);
  EXPECT_EQ(rects[0].w, 96);
  EXPECT_EQ(rects[1].x, 104);
  EXPECT_EQ(rects[1].w, 96);
  EXPECT_EQ(rects[2].x, 0);
  EXPECT_EQ(rects[2].y, 18);
  EXPECT_EQ(rects[2].w, 200);
}

TEST_F(LayoutTest, shrink_height_wrapped_row_includes_line_gaps)
{
  Style container_style;
  container_style.width = 100;
  container_style.height = Style::Size(Style::Size::Mode::SHRINK);
  container_style.layout = Style::Layout{};
  container_style.layout->direction = LayoutDirection::ROW;
  container_style.layout->wrap = Style::Layout::Wrap::LINE;
  container_style.layout->line_gap = 5;
  container_style.layout->gap = Style::Layout::Gap{};
  container_style.layout->gap->mode = Style::Layout::GapMode::FIXED;
  container_style.layout->gap->size = 10;

  auto container = make_ctx(std::move(container_style));
  for (int i = 0; i < 3; i++)
  {
    Style child_style;
    child_style.width = 40;
    child_style.height = 12;
    container->children.push_back(make_ctx(std::move(child_style)));
  }

  Pixils::UI::layout_view_tree(container, {0, 0, 320, 200}, runtime, hook_ctx_val);

  ASSERT_EQ(container->children.size(), 3u);
  EXPECT_EQ(container->bounds.w, 100);
  EXPECT_EQ(container->bounds.h, 29);
  EXPECT_EQ(container->children[2]->bounds.x, 0);
  EXPECT_EQ(container->children[2]->bounds.y, 17);
}

TEST_F(LayoutTest, shrink_height_column_includes_fixed_gaps)
{
  Style container_style;
  container_style.width = 100;
  container_style.height = Style::Size(Style::Size::Mode::SHRINK);
  container_style.layout = Style::Layout{};
  container_style.layout->direction = LayoutDirection::COLUMN;
  container_style.layout->gap = Style::Layout::Gap{};
  container_style.layout->gap->mode = Style::Layout::GapMode::FIXED;
  container_style.layout->gap->size = 8;

  auto container = make_ctx(std::move(container_style));
  container->children.push_back(make_fixed_ctx(20));
  container->children.push_back(make_fixed_ctx(30));

  Pixils::UI::layout_view_tree(container, {0, 0, 320, 200}, runtime, hook_ctx_val);

  ASSERT_EQ(container->children.size(), 2u);
  EXPECT_EQ(container->bounds.h, 58);
  EXPECT_EQ(container->children[0]->bounds.y, 0);
  EXPECT_EQ(container->children[0]->bounds.h, 20);
  EXPECT_EQ(container->children[1]->bounds.y, 28);
  EXPECT_EQ(container->children[1]->bounds.h, 30);

  auto content = container->effective_style.content_rect(container->bounds);
  EXPECT_LE(container->children[1]->bounds.y + container->children[1]->bounds.h,
            content.y + content.h);
}
