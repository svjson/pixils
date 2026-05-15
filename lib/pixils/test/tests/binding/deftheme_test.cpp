#include "../fixture.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/program.h>
#include <pixils/ui/theme.h>

#include <gtest/gtest.h>
#include <lisple/host/object.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using DefThemeTest = BaseFixture;

namespace
{
  Pixils::UI::Theme& get_theme(Lisple::Runtime& rt, const std::string& name)
  {
    auto val = rt.eval("(get pixils/themes '" + name + ")");
    return Lisple::obj<Pixils::UI::Theme>(*val);
  }

  Pixils::UI::ThemeSelector component_selector_with_pseudos(const std::string& value,
                                                            bool hovered = false,
                                                            bool focused = false,
                                                            bool focus_within = false)
  {
    auto selector = Pixils::UI::ThemeSelector::component_type(value);
    selector.hovered = hovered;
    selector.focused = focused;
    selector.focus_within = focus_within;
    return selector;
  }

  Pixils::UI::ThemeSelector class_selector_with_pseudos(const std::string& value,
                                                        bool hovered = false,
                                                        bool focused = false,
                                                        bool focus_within = false)
  {
    auto selector = Pixils::UI::ThemeSelector::class_name(value);
    selector.hovered = hovered;
    selector.focused = focused;
    selector.focus_within = focus_within;
    return selector;
  }
} // namespace

TEST_F(DefThemeTest, deftheme_with_component_and_class_selectors_is_created)
{
  runtime.eval(R"(
    (pixils/deftheme test-theme
      {:styles {'ui/text {:text {:scale 2}}
                :menu/item {:text {:font :font/console}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "test-theme");
  EXPECT_EQ(theme.name, "test-theme");

  auto text_node = theme.get_style(Pixils::UI::ThemeSelector::component_type("ui/text"));
  ASSERT_NE(text_node, nullptr);
  ASSERT_TRUE(text_node->text.has_value());
  ASSERT_TRUE(text_node->text->scale.has_value());
  EXPECT_EQ(*text_node->text->scale, 2);

  auto menu_item = theme.get_style(Pixils::UI::ThemeSelector::class_name("menu/item"));
  ASSERT_NE(menu_item, nullptr);
  ASSERT_TRUE(menu_item->text.has_value());
  ASSERT_TRUE(menu_item->text->font.has_value());
  EXPECT_EQ(*menu_item->text->font, "font/console");
}

TEST_F(DefThemeTest, deftheme_with_compound_state_selector_is_created)
{
  runtime.eval(R"(
    (pixils/deftheme test-theme
      {:styles {'(button {:pressed true})
                {:text {:scale 3}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "test-theme");
  ASSERT_EQ(theme.rules.size(), 1u);
  EXPECT_EQ(theme.rules[0].selector.type, Pixils::UI::ThemeSelector::Type::COMPOUND);
  ASSERT_EQ(theme.rules[0].selector.children.size(), 2u);
  EXPECT_EQ(theme.rules[0].selector.children[0].type,
            Pixils::UI::ThemeSelector::Type::COMPONENT_TYPE);
  EXPECT_EQ(theme.rules[0].selector.children[0].value, "button");
  EXPECT_EQ(theme.rules[0].selector.children[1].type,
            Pixils::UI::ThemeSelector::Type::STATE);
  ASSERT_NE(theme.rules[0].selector.children[1].state, nullptr);
  EXPECT_EQ(theme.rules[0].selector.children[1].state->type, Lisple::Value::Type::MAP);
  EXPECT_EQ(theme.rules[0].selector.children[1].state->to_string(), "{:pressed true}");

  auto pressed_state =
    Lisple::map({Lisple::keyword("pressed"), Lisple::Constant::BOOL_TRUE});
  auto parsed_keys = Lisple::Dict::keys(*theme.rules[0].selector.children[1].state);
  auto manual_keys = Lisple::Dict::keys(*pressed_state);
  ASSERT_EQ(parsed_keys.size(), 1u);
  ASSERT_EQ(manual_keys.size(), 1u);
  EXPECT_EQ(parsed_keys[0]->type, manual_keys[0]->type);
  EXPECT_EQ(parsed_keys[0]->to_string(), manual_keys[0]->to_string());
  auto expected_pressed =
    Lisple::Dict::get_property(theme.rules[0].selector.children[1].state, parsed_keys[0]);
  auto parsed_pressed =
    Lisple::Dict::get_property(theme.rules[0].selector.children[1].state, *parsed_keys[0]);
  auto cross_pressed = Lisple::Dict::get_property(pressed_state, *parsed_keys[0]);
  auto manual_pressed = Lisple::Dict::get_property(pressed_state, *manual_keys[0]);
  ASSERT_NE(expected_pressed, nullptr);
  ASSERT_NE(parsed_pressed, nullptr);
  ASSERT_NE(cross_pressed, nullptr);
  ASSERT_NE(manual_pressed, nullptr);
  EXPECT_EQ(expected_pressed->type, Lisple::Value::Type::BOOL);
  EXPECT_EQ(parsed_pressed->type, Lisple::Value::Type::BOOL);
  EXPECT_EQ(cross_pressed->type, Lisple::Value::Type::BOOL);
  EXPECT_EQ(parsed_pressed->to_string(), manual_pressed->to_string());
  EXPECT_EQ(cross_pressed->to_string(), manual_pressed->to_string());
  EXPECT_TRUE(theme.rules[0].selector.children[1].matches(
    Pixils::UI::ThemeMatchContext{.mode_names = {"button"},
                                  .class_names = {},
                                  .state = pressed_state,
                                  .interaction = {}}));
  auto button_pressed = Pixils::UI::ThemeSelector::compound(
    {Pixils::UI::ThemeSelector::component_type("button"),
     Pixils::UI::ThemeSelector::state_match(pressed_state)});
  EXPECT_TRUE(theme.rules[0].selector == button_pressed);

  auto style = theme.get_style(button_pressed);
  ASSERT_NE(style, nullptr);
  ASSERT_TRUE(style->text.has_value());
  ASSERT_TRUE(style->text->scale.has_value());
  EXPECT_EQ(*style->text->scale, 3);
}

TEST_F(DefThemeTest, component_selector_matches_modes_that_extend_the_component)
{
  runtime.eval(R"(
    (pixils/deftheme test-theme
      {:styles {'button {:text {:scale 2}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "test-theme");
  auto matches = theme.get_matching_styles(
    Pixils::UI::ThemeMatchContext{.mode_names = {"board-button", "button"},
                                  .class_names = {},
                                  .state = Lisple::Constant::NIL,
                                  .interaction = {}});

  ASSERT_EQ(matches.size(), 1u);
  ASSERT_TRUE(matches[0]->text.has_value());
  ASSERT_TRUE(matches[0]->text->scale.has_value());
  EXPECT_EQ(*matches[0]->text->scale, 2);
}

TEST_F(DefThemeTest, class_selector_matches_runtime_view_classes)
{
  runtime.eval(R"(
    (pixils/deftheme test-theme
      {:styles {:ui/panel {:text {:scale 4}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "test-theme");
  auto matches =
    theme.get_matching_styles(Pixils::UI::ThemeMatchContext{.mode_names = {"game-mode"},
                                                            .class_names = {"ui/panel"},
                                                            .state = Lisple::Constant::NIL,
                                                            .interaction = {}});

  ASSERT_EQ(matches.size(), 1u);
  ASSERT_TRUE(matches[0]->text.has_value());
  ASSERT_TRUE(matches[0]->text->scale.has_value());
  EXPECT_EQ(*matches[0]->text->scale, 4);
}

TEST_F(DefThemeTest, focus_pseudo_state_selectors_match_interaction_state)
{
  runtime.eval(R"(
    (pixils/deftheme test-theme
      {:styles {'window:focus-within {:text {:scale 4}}
                'input:focus {:text {:scale 2}}
                :ui/menu-item:focus {:text {:scale 6}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "test-theme");

  EXPECT_NE(theme.get_style(component_selector_with_pseudos("window", false, false, true)),
            nullptr);
  EXPECT_NE(theme.get_style(component_selector_with_pseudos("input", false, true, false)),
            nullptr);
  EXPECT_NE(theme.get_style(class_selector_with_pseudos("ui/menu-item", false, true, false)),
            nullptr);

  Pixils::UI::InteractionState focus_within_interaction;
  focus_within_interaction.focus_within = true;
  auto window_matches = theme.get_matching_styles(
    Pixils::UI::ThemeMatchContext{.mode_names = {"window"},
                                  .class_names = {},
                                  .state = Lisple::map({}),
                                  .interaction = focus_within_interaction});

  ASSERT_EQ(window_matches.size(), 1u);
  ASSERT_TRUE(window_matches[0]->text.has_value());
  ASSERT_TRUE(window_matches[0]->text->scale.has_value());
  EXPECT_EQ(*window_matches[0]->text->scale, 4);

  Pixils::UI::InteractionState focused_interaction;
  focused_interaction.focused = true;
  focused_interaction.focus_within = true;
  auto input_matches = theme.get_matching_styles(
    Pixils::UI::ThemeMatchContext{.mode_names = {"input"},
                                  .class_names = {},
                                  .state = Lisple::map({}),
                                  .interaction = focused_interaction});

  ASSERT_EQ(input_matches.size(), 1u);
  ASSERT_TRUE(input_matches[0]->text.has_value());
  ASSERT_TRUE(input_matches[0]->text->scale.has_value());
  EXPECT_EQ(*input_matches[0]->text->scale, 2);

  auto menu_item_matches = theme.get_matching_styles(
    Pixils::UI::ThemeMatchContext{.mode_names = {"menu-item"},
                                  .class_names = {"ui/menu-item"},
                                  .state = Lisple::map({}),
                                  .interaction = focused_interaction});

  ASSERT_EQ(menu_item_matches.size(), 1u);
  ASSERT_TRUE(menu_item_matches[0]->text.has_value());
  ASSERT_TRUE(menu_item_matches[0]->text->scale.has_value());
  EXPECT_EQ(*menu_item_matches[0]->text->scale, 6);
}

TEST_F(DefThemeTest, descendant_selectors_match_ancestor_focus_chain)
{
  runtime.eval(R"(
    (pixils/deftheme test-theme
      {:styles {['window:focus-within 'window-title-bar] {:text {:scale 5}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "test-theme");

  Pixils::UI::InteractionState focus_within_interaction;
  focus_within_interaction.focus_within = true;
  auto matches = theme.get_matching_styles(std::vector<Pixils::UI::ThemeMatchContext>{
    Pixils::UI::ThemeMatchContext{.mode_names = {"window"},
                                  .class_names = {},
                                  .state = Lisple::map({}),
                                  .interaction = focus_within_interaction},
    Pixils::UI::ThemeMatchContext{.mode_names = {"window-title-bar"},
                                  .class_names = {},
                                  .state = Lisple::map({}),
                                  .interaction = {}}});

  ASSERT_EQ(matches.size(), 1u);
  ASSERT_TRUE(matches[0]->text.has_value());
  ASSERT_TRUE(matches[0]->text->scale.has_value());
  EXPECT_EQ(*matches[0]->text->scale, 5);

  auto non_matching = theme.get_matching_styles(std::vector<Pixils::UI::ThemeMatchContext>{
    Pixils::UI::ThemeMatchContext{.mode_names = {"window"},
                                  .class_names = {},
                                  .state = Lisple::map({}),
                                  .interaction = {}},
    Pixils::UI::ThemeMatchContext{.mode_names = {"window-title-bar"},
                                  .class_names = {},
                                  .state = Lisple::map({}),
                                  .interaction = {}}});

  EXPECT_TRUE(non_matching.empty());
}

TEST_F(DefThemeTest, deftheme_extend_merges_parent_styles_and_overrides_them)
{
  runtime.eval(R"(
    (pixils/deftheme base-theme
      {:styles {'ui/text {:text {:font :font/console
                                   :scale 1}}
                :menu/item {:text {:color {:r 0 :g 0 :b 0 :a 255}}}}})

    (pixils/deftheme child-theme
      {:extend 'base-theme
       :styles {'ui/text {:text {:scale 3}}
                :status/panel {:text {:font :font/status}}}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "child-theme");
  ASSERT_EQ(theme.extend.size(), 1u);
  EXPECT_EQ(theme.extend[0], "base-theme");

  auto text_node = theme.get_style(Pixils::UI::ThemeSelector::component_type("ui/text"));
  ASSERT_NE(text_node, nullptr);
  ASSERT_TRUE(text_node->text.has_value());
  ASSERT_TRUE(text_node->text->font.has_value());
  ASSERT_TRUE(text_node->text->scale.has_value());
  EXPECT_EQ(*text_node->text->font, "font/console");
  EXPECT_EQ(*text_node->text->scale, 3);

  auto menu_item = theme.get_style(Pixils::UI::ThemeSelector::class_name("menu/item"));
  ASSERT_NE(menu_item, nullptr);
  ASSERT_TRUE(menu_item->text.has_value());
  ASSERT_TRUE(menu_item->text->color.has_value());
  EXPECT_EQ(menu_item->text->color->r, 0);

  auto status_panel = theme.get_style(Pixils::UI::ThemeSelector::class_name("status/panel"));
  ASSERT_NE(status_panel, nullptr);
  ASSERT_TRUE(status_panel->text.has_value());
  ASSERT_TRUE(status_panel->text->font.has_value());
  EXPECT_EQ(*status_panel->text->font, "font/status");
}

TEST_F(DefThemeTest, deftheme_can_compose_visual_and_layout_theme_layers)
{
  runtime.eval(R"(
    (pixils/deftheme visual-theme
      {:styles {:ui/panel {:background {:r 1 :g 2 :b 3 :a 255}
                           :text {:font :font/visual}}}})

    (pixils/deftheme compact-layout-theme
      {:styles {:ui/panel {:padding [2 4]
                           :layout {:direction :row
                                    :gap 3}}}})

    (pixils/deftheme compact-visual-theme
      {:extend ['visual-theme 'compact-layout-theme]
       :styles {}})
  )");

  Pixils::UI::Theme& theme = get_theme(runtime, "compact-visual-theme");
  ASSERT_EQ(theme.extend.size(), 2u);
  EXPECT_EQ(theme.extend[0], "visual-theme");
  EXPECT_EQ(theme.extend[1], "compact-layout-theme");

  auto panel = theme.get_style(Pixils::UI::ThemeSelector::class_name("ui/panel"));
  ASSERT_NE(panel, nullptr);

  ASSERT_TRUE(panel->background.has_value());
  ASSERT_TRUE(panel->background->color.has_value());
  EXPECT_EQ(panel->background->color->r, 1);
  EXPECT_EQ(panel->background->color->g, 2);
  EXPECT_EQ(panel->background->color->b, 3);

  ASSERT_TRUE(panel->text.has_value());
  ASSERT_TRUE(panel->text->font.has_value());
  EXPECT_EQ(*panel->text->font, "font/visual");

  ASSERT_TRUE(panel->padding.has_value());
  EXPECT_EQ(panel->padding->t, 2);
  EXPECT_EQ(panel->padding->r, 4);
  EXPECT_EQ(panel->padding->b, 2);
  EXPECT_EQ(panel->padding->l, 4);

  ASSERT_TRUE(panel->layout.has_value());
  ASSERT_TRUE(panel->layout->direction.has_value());
  ASSERT_TRUE(panel->layout->gap.has_value());
  ASSERT_TRUE(panel->layout->gap->mode.has_value());
  ASSERT_TRUE(panel->layout->gap->size.has_value());
  EXPECT_EQ(*panel->layout->direction, Pixils::UI::LayoutDirection::ROW);
  EXPECT_EQ(*panel->layout->gap->mode, Pixils::UI::Style::Layout::GapMode::FIXED);
  EXPECT_EQ(*panel->layout->gap->size, 3);
}

TEST_F(DefThemeTest, defprogram_with_theme_is_created)
{
  runtime.eval(R"(
    (pixils/deftheme app-theme {:styles {}})
    (pixils/defprogram app {:initial-mode 'root-mode
                            :theme 'app-theme})
  )");

  auto program_val = runtime.eval("(get pixils/programs 'app)");
  Pixils::Program& program = Lisple::obj<Pixils::Program>(*program_val);
  ASSERT_TRUE(program.theme.has_value());
  ASSERT_EQ(program.theme->size(), 1u);
  EXPECT_EQ((*program.theme)[0], "app-theme");
}

TEST_F(DefThemeTest, defprogram_accepts_theme_vectors)
{
  runtime.eval(R"(
    (pixils/deftheme visual-theme {:styles {}})
    (pixils/deftheme compact-layout-theme {:styles {}})
    (pixils/defprogram app {:initial-mode 'root-mode
                            :theme ['visual-theme 'compact-layout-theme]})
  )");

  auto program_val = runtime.eval("(get pixils/programs 'app)");
  Pixils::Program& program = Lisple::obj<Pixils::Program>(*program_val);
  ASSERT_TRUE(program.theme.has_value());
  ASSERT_EQ(program.theme->size(), 2u);
  EXPECT_EQ((*program.theme)[0], "visual-theme");
  EXPECT_EQ((*program.theme)[1], "compact-layout-theme");
}

TEST_F(DefThemeTest, defprogram_with_target_frame_rate_is_created)
{
  runtime.eval(R"(
    (pixils/defprogram app {:initial-mode 'root-mode
                            :target-frame-rate 144})
  )");

  auto program_val = runtime.eval("(get pixils/programs 'app)");
  Pixils::Program& program = Lisple::obj<Pixils::Program>(*program_val);
  EXPECT_EQ(program.target_frame_rate, 144);
}

TEST_F(DefThemeTest, defprogram_target_frame_rate_can_disable_pacing_with_zero)
{
  runtime.eval(R"(
    (pixils/defprogram app {:initial-mode 'root-mode
                            :target-frame-rate 0})
  )");

  auto program_val = runtime.eval("(get pixils/programs 'app)");
  Pixils::Program& program = Lisple::obj<Pixils::Program>(*program_val);
  EXPECT_EQ(program.target_frame_rate, 0);
}
