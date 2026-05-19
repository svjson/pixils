#include "../support/benchmark.h"

#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/geom.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>
#include <pixils/script.h>
#include <pixils/ui/style.h>
#include <pixils/ui/view_lifecycle.h>
#include <pixils/ui/view_layout.h>

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace
{
  using Pixils::Rect;
  using Pixils::Runtime::Mode;
  using Pixils::Runtime::View;
  using Pixils::UI::LayoutDirection;
  using Pixils::UI::Style;

  class LayoutBenchmark : public ::testing::Test
  {
   protected:
    Pixils::RenderContext render_ctx{};
    Lisple::Runtime runtime;
    Pixils::FrameEvents events;
    Pixils::HookContext hook_ctx;
    Lisple::sptr_val hook_ctx_val;

    LayoutBenchmark()
      : runtime(Pixils::init_lisple_runtime(render_ctx, "benchmark", {}))
      , hook_ctx{&events, &render_ctx}
      , hook_ctx_val(Pixils::Script::HookContextAdapter::make_ref(hook_ctx))
    {
      render_ctx.buffer_dim = {320, 200};
    }
  };

  std::shared_ptr<View> make_view(std::optional<Style> style = std::nullopt)
  {
    auto view = std::make_shared<View>();
    view->owned_mode = std::make_unique<Mode>();
    view->owned_mode->style = std::move(style);
    view->mode = view->owned_mode.get();
    view->state = Lisple::Constant::NIL;
    return view;
  }

  std::shared_ptr<View> make_fixed_view(int width, int height)
  {
    Style style;
    style.width = width;
    style.height = height;
    return make_view(std::move(style));
  }

  std::shared_ptr<View> make_fill_view()
  {
    Style style;
    style.width = Style::Size(Style::Size::Mode::FILL);
    style.height = Style::Size(Style::Size::Mode::FILL);
    return make_view(std::move(style));
  }

  std::shared_ptr<View> make_shrink_view(int child_width, int child_height)
  {
    Style style;
    style.width = Style::Size(Style::Size::Mode::SHRINK);
    style.height = Style::Size(Style::Size::Mode::SHRINK);
    auto view = make_view(std::move(style));
    view->children.push_back(make_fixed_view(child_width, child_height));
    return view;
  }

  std::shared_ptr<View> make_absolute_view(int width, int height, int left, int top)
  {
    Style style;
    style.width = width;
    style.height = height;
    style.position = Pixils::UI::PositionMode::ABSOLUTE;
    style.left = left;
    style.top = top;
    return make_view(std::move(style));
  }

  std::shared_ptr<View> make_visibility_none_view()
  {
    Style style;
    style.visibility = Style::Visibility::NONE;
    style.width = 20;
    style.height = 10;
    return make_view(std::move(style));
  }

  std::vector<std::shared_ptr<View>> make_mixed_children(std::size_t count)
  {
    std::vector<std::shared_ptr<View>> children;
    children.reserve(count);
    for (std::size_t i = 0; i < count; i++)
    {
      switch (i % 3)
      {
      case 0:
        children.push_back(make_fixed_view(24 + static_cast<int>(i % 7), 12));
        break;
      case 1:
        children.push_back(make_fill_view());
        break;
      default:
        children.push_back(make_shrink_view(16 + static_cast<int>(i % 11), 10));
        break;
      }
    }
    return children;
  }

  std::shared_ptr<View> make_root_with_layout(LayoutDirection direction)
  {
    Style style;
    style.width = Style::Size(Style::Size::Mode::FILL);
    style.height = Style::Size(Style::Size::Mode::FILL);
    style.layout = Style::Layout{};
    style.layout->direction = direction;
    auto root = make_view(std::move(style));
    return root;
  }

  std::shared_ptr<View> make_deep_chain(std::size_t depth)
  {
    auto root = make_root_with_layout(LayoutDirection::COLUMN);
    auto current = root;
    for (std::size_t i = 0; i < depth; i++)
    {
      Style style;
      style.width = Style::Size(Style::Size::Mode::FILL);
      style.height = i + 1 == depth ? Style::Size(12) : Style::Size(Style::Size::Mode::FILL);
      style.padding = Style::Insets(1, 1);
      style.layout = Style::Layout{};
      style.layout->direction = LayoutDirection::COLUMN;
      auto child = make_view(std::move(style));
      current->children.push_back(child);
      current = child;
    }
    Pixils::UI::attach_style_view_tree(root, nullptr);
    return root;
  }

  std::shared_ptr<View> make_wide_tree(std::size_t count)
  {
    auto root = make_root_with_layout(LayoutDirection::ROW);
    root->children = make_mixed_children(count);
    Pixils::UI::attach_style_view_tree(root, nullptr);
    return root;
  }

  std::shared_ptr<View> make_absolute_tree(std::size_t count)
  {
    auto root = make_root_with_layout(LayoutDirection::COLUMN);
    root->children.reserve(count);
    for (std::size_t i = 0; i < count; i++)
    {
      root->children.push_back(make_absolute_view(20,
                                                  10,
                                                  static_cast<int>((i * 3) % 300),
                                                  static_cast<int>((i * 5) % 180)));
    }
    Pixils::UI::attach_style_view_tree(root, nullptr);
    return root;
  }

  std::shared_ptr<View> make_hidden_tree(std::size_t count)
  {
    auto root = make_root_with_layout(LayoutDirection::COLUMN);
    root->children.reserve(count);
    for (std::size_t i = 0; i < count; i++)
    {
      root->children.push_back(i % 2 == 0 ? make_visibility_none_view()
                                          : make_fixed_view(20, 10));
    }
    Pixils::UI::attach_style_view_tree(root, nullptr);
    return root;
  }
} // namespace

TEST_F(LayoutBenchmark, layout_120_mixed_children)
{
  auto children = make_mixed_children(120);
  Rect parent = {0, 0, 640, 480};
  Style::Layout layout;
  layout.direction = LayoutDirection::ROW;
  layout.gap = Style::Layout::Gap{};
  layout.gap->mode = Style::Layout::GapMode::FIXED;
  layout.gap->size = 2;

  Pixils::Benchmark::Case("layout_120_mixed_children",
                          Pixils::Benchmark::layout_category())
    .with_iterations(1000)
    .run(
      [&]()
      {
        auto rects =
          Pixils::UI::layout_children(children, parent, runtime, hook_ctx_val, layout);
        Pixils::Benchmark::consume(static_cast<std::int64_t>(rects.size()));
        Pixils::Benchmark::consume(rects.empty() ? 0 : rects.back().w);
      });
}

TEST_F(LayoutBenchmark, clinical_wide_500_mixed_children)
{
  auto root = make_wide_tree(500);
  Rect bounds = {0, 0, 1200, 700};

  Pixils::Benchmark::Case("clinical_layout_wide_500_mixed_children",
                          Pixils::Benchmark::layout_category())
    .with_iterations(250)
    .run(
      [&]()
      {
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(root->children.empty()
                                     ? 0
                                     : root->children.back()->bounds.w);
      });
}

TEST_F(LayoutBenchmark, clinical_deep_80_nested_chain)
{
  auto root = make_deep_chain(80);
  Rect bounds = {0, 0, 640, 480};

  Pixils::Benchmark::Case("clinical_layout_deep_80_nested_chain",
                          Pixils::Benchmark::layout_category())
    .with_iterations(500)
    .run(
      [&]()
      {
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(root->external_bounds.h);
      });
}

TEST_F(LayoutBenchmark, clinical_200_absolute_children)
{
  auto root = make_absolute_tree(200);
  Rect bounds = {0, 0, 640, 480};

  Pixils::Benchmark::Case("clinical_layout_200_absolute_children",
                          Pixils::Benchmark::layout_category())
    .with_iterations(500)
    .run(
      [&]()
      {
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(root->children.back()->bounds.x);
      });
}

TEST_F(LayoutBenchmark, clinical_200_mixed_hidden_none_children)
{
  auto root = make_hidden_tree(200);
  Rect bounds = {0, 0, 640, 480};

  Pixils::Benchmark::Case("clinical_layout_200_mixed_hidden_none_children",
                          Pixils::Benchmark::layout_category())
    .with_iterations(500)
    .run(
      [&]()
      {
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(root->children.back()->bounds.h);
      });
}

TEST_F(LayoutBenchmark, clinical_stable_tree_repeated_layout)
{
  auto root = make_wide_tree(240);
  Rect bounds = {0, 0, 1024, 768};

  Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);

  Pixils::Benchmark::Case("clinical_layout_stable_tree_repeated_layout",
                          Pixils::Benchmark::layout_category())
    .with_iterations(1000)
    .run(
      [&]()
      {
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(root->children.front()->bounds.w);
      });
}
