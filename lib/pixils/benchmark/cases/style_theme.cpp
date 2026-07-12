#include "../support/benchmark.h"
#include <pixils/color.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>
#include <pixils/script.h>
#include <pixils/ui/style.h>
#include <pixils/ui/theme.h>
#include <pixils/ui/view_layout.h>
#include <pixils/ui/view_lifecycle.h>

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
  using Pixils::Rect;
  using Pixils::Runtime::Mode;
  using Pixils::Runtime::View;
  using Pixils::UI::InteractionState;
  using Pixils::UI::LayoutDirection;
  using Pixils::UI::Style;
  using Pixils::UI::Theme;
  using Pixils::UI::ThemeMatchContext;
  using Pixils::UI::ThemeSelector;

  class StyleThemeBenchmark : public ::testing::Test
  {
   protected:
    Pixils::RenderContext render_ctx{};
    Roo::Runtime runtime;
    Pixils::FrameEvents events;
    Pixils::HookContext hook_ctx;
    Roo::sptr_val hook_ctx_val;

    StyleThemeBenchmark()
      : runtime(Pixils::init_roo_runtime(render_ctx, "benchmark", {}))
      , hook_ctx{&events, &render_ctx}
      , hook_ctx_val(Pixils::Script::HookContextAdapter::make_ref(hook_ctx))
    {
      render_ctx.buffer_dim = {1024, 768};
    }
  };

  Style fixed_style(int width, int height)
  {
    Style style;
    style.width = width;
    style.height = height;
    style.padding = Style::Insets(2, 2);
    return style;
  }

  Style root_style()
  {
    Style style;
    style.width = Style::Size(Style::Size::Mode::FILL);
    style.height = Style::Size(Style::Size::Mode::FILL);
    style.layout = Style::Layout{};
    style.layout->direction = LayoutDirection::COLUMN;
    style.layout->gap = Style::Layout::Gap{};
    style.layout->gap->mode = Style::Layout::GapMode::FIXED;
    style.layout->gap->size = 1;
    return style;
  }

  Style themed_style(int index)
  {
    Style style;
    style.padding = Style::Insets(index % 4, (index + 1) % 4);
    style.margin = Style::Insets(index % 3, index % 5, (index + 2) % 3, index % 7);
    style.background =
      Style::Background(Pixils::Color(static_cast<uint8_t>((index * 17) % 255),
                                      static_cast<uint8_t>((index * 29) % 255),
                                      static_cast<uint8_t>((index * 41) % 255)));
    style.text = Style::Text{};
    style.text->scale = 1 + (index % 3);
    return style;
  }

  std::shared_ptr<View> make_view(std::string name,
                                  std::vector<std::string> classes = {},
                                  std::optional<Style> style = std::nullopt)
  {
    auto view = std::make_shared<View>();
    view->owned_mode = std::make_unique<Mode>();
    view->owned_mode->name = name;
    view->owned_mode->selector_modes = {std::move(name)};
    view->owned_mode->class_names = std::move(classes);
    view->owned_mode->style = std::move(style);
    view->mode = view->owned_mode.get();
    view->state = Roo::map({});
    return view;
  }

  std::shared_ptr<View> make_themed_tree(std::size_t rows, std::size_t columns, Theme theme)
  {
    auto root = make_view("clinical-root", {"ui/panel"}, root_style());
    root->inherited_theme = std::move(theme);
    root->children.reserve(rows);

    for (std::size_t row = 0; row < rows; row++)
    {
      auto row_view = make_view("clinical-row",
                                {"row", "row-" + std::to_string(row % 8)},
                                fixed_style(960, 18));
      row_view->owned_mode->style->layout = Style::Layout{};
      row_view->owned_mode->style->layout->direction = LayoutDirection::ROW;
      row_view->owned_mode->style->layout->gap = Style::Layout::Gap{};
      row_view->owned_mode->style->layout->gap->mode = Style::Layout::GapMode::FIXED;
      row_view->owned_mode->style->layout->gap->size = 2;
      row_view->children.reserve(columns);

      for (std::size_t column = 0; column < columns; column++)
      {
        auto cell = make_view("clinical-cell",
                              {"cell",
                               "column-" + std::to_string(column % 12),
                               row % 2 == 0 ? "even-row" : "odd-row"},
                              fixed_style(24 + static_cast<int>(column % 5), 16));
        if ((row + column) % 9 == 0)
        {
          cell->state =
            Roo::map({Roo::keyword("selected"), Roo::Constant::BOOL_TRUE});
        }
        row_view->children.push_back(cell);
      }

      root->children.push_back(row_view);
    }

    Pixils::UI::attach_style_view_tree(root, nullptr);
    return root;
  }

  ThemeSelector hovered(ThemeSelector selector)
  {
    selector.hovered = true;
    return selector;
  }

  Theme make_clinical_theme(std::size_t rule_count)
  {
    Theme theme;
    theme.name = "clinical-benchmark-theme";
    theme.defaults = Style{};
    theme.defaults->text = Style::Text{};
    theme.defaults->text->font = "font/clinical";
    theme.defaults->text->scale = 1;

    for (std::size_t i = 0; i < rule_count; i++)
    {
      Style style = themed_style(static_cast<int>(i));
      switch (i % 6)
      {
      case 0:
        theme.set_style(ThemeSelector::component_type("clinical-cell"), style);
        break;
      case 1:
        theme.set_style(ThemeSelector::class_name("column-" + std::to_string(i % 12)),
                        style);
        break;
      case 2:
        theme.set_style(ThemeSelector::compound(
                          {ThemeSelector::component_type("clinical-cell"),
                           ThemeSelector::class_name(i % 2 == 0 ? "even-row" : "odd-row")}),
                        style);
        break;
      case 3:
        theme.set_style(ThemeSelector::state_match(Roo::map(
                          {Roo::keyword("selected"), Roo::Constant::BOOL_TRUE})),
                        style);
        break;
      case 4:
        theme.set_style(
          ThemeSelector::descendant({ThemeSelector::component_type("clinical-root"),
                                     ThemeSelector::component_type("clinical-row"),
                                     ThemeSelector::class_name("cell")}),
          style);
        break;
      default:
        theme.set_style(hovered(ThemeSelector::class_name("cell")), style);
        break;
      }
    }

    return theme;
  }

  std::vector<ThemeMatchContext> clinical_selector_path(bool hovered_cell = false)
  {
    InteractionState root_state;
    root_state.focus_within = true;
    InteractionState hover_state;
    hover_state.hovered = hovered_cell;
    return {ThemeMatchContext{.mode_names = {"clinical-root"},
                              .class_names = {"ui/panel"},
                              .state = Roo::map({}),
                              .interaction = root_state},
            ThemeMatchContext{.mode_names = {"clinical-row"},
                              .class_names = {"row", "row-2"},
                              .state = Roo::map({}),
                              .interaction = {}},
            ThemeMatchContext{.mode_names = {"clinical-cell"},
                              .class_names = {"cell", "column-6", "even-row"},
                              .state = Roo::map(
                                {Roo::keyword("selected"), Roo::Constant::BOOL_TRUE}),
                              .interaction = hover_state}};
  }

  std::shared_ptr<View> pick_leaf(const std::shared_ptr<View>& root)
  {
    auto current = root;
    while (current && !current->children.empty())
    {
      current = current->children.back();
    }
    return current;
  }
} // namespace

TEST_F(StyleThemeBenchmark, clinical_theme_lookup_360_mixed_rules)
{
  auto theme = make_clinical_theme(360);
  auto selector_path = clinical_selector_path(true);

  Pixils::Benchmark::Case("clinical_theme_lookup_360_mixed_rules",
                          Pixils::Benchmark::ui_category())
    .with_iterations(1000)
    .run(
      [&]()
      {
        auto matches = theme.get_matching_styles(selector_path);
        Pixils::Benchmark::consume(static_cast<std::int64_t>(matches.size()));
      });
}

TEST_F(StyleThemeBenchmark, clinical_style_resolve_layered_variants)
{
  Style base = themed_style(12);
  base.hover = std::make_unique<Style>(themed_style(13));
  base.focus_within = std::make_unique<Style>(themed_style(14));
  base.focus = std::make_unique<Style>(themed_style(15));

  Style inherited;
  inherited.text = Style::Text{};
  inherited.text->font = "font/inherited";
  inherited.text->scale = 2;

  Style defaults = themed_style(16);
  InteractionState interaction;
  interaction.hovered = true;
  interaction.focus_within = true;
  interaction.focused = true;

  Pixils::Benchmark::Case("clinical_style_resolve_layered_variants",
                          Pixils::Benchmark::ui_category())
    .with_iterations(2000)
    .run(
      [&]()
      {
        auto resolved = Pixils::UI::resolve_style(base,
                                                  &inherited,
                                                  Roo::map({}),
                                                  interaction,
                                                  &defaults);
        Pixils::Benchmark::consume(
          resolved.text && resolved.text->scale ? *resolved.text->scale : 0);
      });
}

TEST_F(StyleThemeBenchmark, clinical_style_theme_cached_tree_repeated_layout)
{
  auto root = make_themed_tree(40, 8, make_clinical_theme(240));
  Rect bounds = {0, 0, 1024, 768};

  Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);

  Pixils::Benchmark::Case("clinical_style_theme_cached_tree_repeated_layout",
                          Pixils::Benchmark::ui_category())
    .with_iterations(500)
    .run(
      [&]()
      {
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(root->children.back()->children.back()->bounds.w);
      });
}

TEST_F(StyleThemeBenchmark, clinical_style_theme_leaf_invalidation_layout)
{
  auto root = make_themed_tree(40, 8, make_clinical_theme(240));
  auto leaf = pick_leaf(root);
  ASSERT_NE(leaf, nullptr);
  Rect bounds = {0, 0, 1024, 768};

  Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);

  Pixils::Benchmark::Case("clinical_style_theme_leaf_invalidation_layout",
                          Pixils::Benchmark::ui_category())
    .with_iterations(500)
    .run(
      [&]()
      {
        leaf->style_view.invalidate();
        leaf->mark_style_changed();
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(
          leaf->effective_style.padding ? leaf->effective_style.padding->l : 0);
      });
}

TEST_F(StyleThemeBenchmark, clinical_style_theme_hover_toggle_layout)
{
  auto root = make_themed_tree(40, 8, make_clinical_theme(240));
  auto leaf = pick_leaf(root);
  ASSERT_NE(leaf, nullptr);
  Rect bounds = {0, 0, 1024, 768};
  bool hovered_cell = false;

  Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);

  Pixils::Benchmark::Case("clinical_style_theme_hover_toggle_layout",
                          Pixils::Benchmark::ui_category())
    .with_iterations(500)
    .run(
      [&]()
      {
        hovered_cell = !hovered_cell;
        leaf->interaction.hovered = hovered_cell;
        leaf->mark_interaction_changed();
        Pixils::UI::layout_view_tree(root, bounds, runtime, hook_ctx_val);
        Pixils::Benchmark::consume(
          leaf->effective_style.margin ? leaf->effective_style.margin->t : 0);
      });
}
