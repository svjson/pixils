#include "../fixture.h"
#include "pixils/binding/component_definition.h"
#include "pixils/runtime/mode.h"

#include <gtest/gtest.h>
#include <roo/exception.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

using DefModeTest = BaseFixture;

namespace
{
  Pixils::Runtime::ViewDefinition& get_definition(Roo::Runtime& rt, const std::string& name)
  {
    auto val = rt.eval("(get pixils/modes '" + name + ")");
    if (!val || val->type == Roo::Value::Type::NIL)
    {
      val = rt.eval("(get pixils/components '" + name + ")");
    }
    return Pixils::Script::definition_from_registry_value(val, "test mode lookup");
  }

  Pixils::Runtime::Component& get_component(Roo::Runtime& rt, const std::string& name)
  {
    auto val = rt.eval("(get pixils/components '" + name + ")");
    auto* component = Pixils::Script::component_from_registry_value(val);
    if (!component)
    {
      throw Roo::InvocationException("test component lookup resolved to non-component");
    }
    return *component;
  }
} // namespace

TEST_F(DefModeTest, defmode_with_no_args_is_created_with_nil_hooks)
{
  // When
  runtime.eval("(pixils/defmode test-mode {})");

  // Then
  auto mode_val = runtime.eval("(get pixils/modes 'test-mode)");
  Pixils::Runtime::Mode& mode = Roo::obj<Pixils::Runtime::Mode>(*mode_val);
  ASSERT_EQ(mode.selector_modes.size(), 1u);
  EXPECT_EQ(mode.selector_modes[0], "test-mode");
  EXPECT_EQ(*mode.init, *Roo::Constant::NIL);
  EXPECT_EQ(*mode.update, *Roo::Constant::NIL);
  EXPECT_EQ(*mode.content_size, *Roo::Constant::NIL);
  EXPECT_EQ(*mode.render, *Roo::Constant::NIL);
}

TEST_F(DefModeTest, defmode_registers_mode_only_in_mode_registry)
{
  runtime.eval("(pixils/defmode registry-mode {})");

  auto mode_val = runtime.eval("(get pixils/modes 'registry-mode)");
  auto component_val = runtime.eval("(get pixils/components 'registry-mode)");

  ASSERT_NE(mode_val, nullptr);
  EXPECT_NE(mode_val->type, Roo::Value::Type::NIL);
  ASSERT_NE(component_val, nullptr);
  EXPECT_EQ(component_val->type, Roo::Value::Type::NIL);
}

TEST_F(DefModeTest, defmode_accepts_a_docstring_before_the_definition)
{
  runtime.eval(R"(
    (pixils/defmode documented-mode
      "A mode documented for external tools."
      {})
  )");

  auto mode_val = runtime.eval("(get pixils/modes 'documented-mode)");
  ASSERT_NE(mode_val, nullptr);
  ASSERT_NE(mode_val->type, Roo::Value::Type::NIL);
  auto& mode = Roo::obj<Pixils::Runtime::Mode>(*mode_val);
  EXPECT_EQ(mode.name, "documented-mode");
  EXPECT_EQ(*mode.render, *Roo::Constant::NIL);
}

TEST_F(DefModeTest, defmode_injects_name_when_top_level_value_matches_name_key)
{
  // When
  runtime.eval("(pixils/defmode test-mode {:render 'name})");

  // Then
  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "test-mode");
  EXPECT_EQ(mode.name, "test-mode");
  ASSERT_EQ(mode.render->type, Roo::Value::Type::SYMBOL);
  EXPECT_EQ(mode.render->str(), "name");
}

TEST_F(DefModeTest, defcomponent_creates_component_definition)
{
  runtime.eval(R"(
    (pixils/defcomponent test-button
      {:init-ui (fn [ui-state state ctx] ui-state)
       :update-ui (fn [ui-state state ctx] ui-state)
       :after-layout-ui (fn [ui-state state ctx] ui-state)
       :ui/state-keys [:pressed]})
  )");

  auto& component = get_component(runtime, "test-button");
  EXPECT_EQ(component.init_ui->type, Roo::Value::Type::FUNCTION);
  EXPECT_EQ(component.update_ui->type, Roo::Value::Type::FUNCTION);
  EXPECT_EQ(component.after_layout_ui->type, Roo::Value::Type::FUNCTION);
  ASSERT_EQ(component.ui_state_keys.size(), 1u);
  EXPECT_EQ(component.ui_state_keys[0], "pressed");
  EXPECT_EQ(component.name, "test-button");

  auto mode_val = runtime.eval("(get pixils/modes 'test-button)");
  ASSERT_NE(mode_val, nullptr);
  EXPECT_EQ(mode_val->type, Roo::Value::Type::NIL);
}

TEST_F(DefModeTest, defcomponent_accepts_a_docstring_before_the_definition)
{
  runtime.eval(R"(
    (pixils/defcomponent documented-component
      "A component documented for external tools."
      {})
  )");

  auto& component = get_component(runtime, "documented-component");
  EXPECT_EQ(component.name, "documented-component");
  EXPECT_EQ(*component.render, *Roo::Constant::NIL);
}

TEST_F(DefModeTest, defcomponent_extend_adds_unique_ui_state_keys)
{
  runtime.eval(R"(
    (pixils/defcomponent base-component
      {:ui/state-keys [:base :shared]})
    (pixils/defcomponent derived-component
      {:extend 'base-component
       :ui/state-keys [:derived :shared]})
  )");

  auto& component = get_component(runtime, "derived-component");
  ASSERT_EQ(component.ui_state_keys.size(), 3u);
  EXPECT_EQ(component.ui_state_keys[0], "base");
  EXPECT_EQ(component.ui_state_keys[1], "shared");
  EXPECT_EQ(component.ui_state_keys[2], "derived");
}

TEST_F(DefModeTest, defmode_rejects_component_ui_state_fields)
{
  EXPECT_THROW(runtime.eval("(pixils/defmode bad-mode {:ui/state-keys [:pressed]})"),
               Roo::TypeError);
  EXPECT_THROW(
    runtime.eval("(pixils/defmode bad-mode {:init-ui (fn [ui-state state ctx] ui-state)})"),
    Roo::TypeError);
  EXPECT_THROW(
    runtime.eval(
      "(pixils/defmode bad-mode {:update-ui (fn [ui-state state ctx] ui-state)})"),
    Roo::TypeError);
  EXPECT_THROW(
    runtime.eval(
      "(pixils/defmode bad-mode {:after-layout-ui (fn [ui-state state ctx] ui-state)})"),
    Roo::TypeError);
}

TEST_F(DefModeTest, defcomponent_rejects_mode_composition)
{
  runtime.eval(R"(
    (pixils/def thing-compose (pixils/make-mode-composition {:render :pass}))
  )");

  EXPECT_THROW(runtime.eval("(pixils/defcomponent bad-component {:compose thing-compose})"),
               Roo::TypeError);
}

TEST_F(DefModeTest, defmode_extend_preserves_selector_ancestry_for_theme_matching)
{
  runtime.eval(R"(
    (pixils/defcomponent button {})
    (pixils/defcomponent board-button {:extend 'button})
    (pixils/defcomponent special-board-button {:extend 'board-button})
  )");

  auto& board_button = get_definition(runtime, "board-button");
  ASSERT_EQ(board_button.selector_modes.size(), 2u);
  EXPECT_EQ(board_button.selector_modes[0], "board-button");
  EXPECT_EQ(board_button.selector_modes[1], "button");

  auto& special_board_button = get_definition(runtime, "special-board-button");
  ASSERT_EQ(special_board_button.selector_modes.size(), 3u);
  EXPECT_EQ(special_board_button.selector_modes[0], "special-board-button");
  EXPECT_EQ(special_board_button.selector_modes[1], "board-button");
  EXPECT_EQ(special_board_button.selector_modes[2], "button");
}

TEST_F(DefModeTest, defmode_class_accepts_keyword_or_vector_and_merges_on_extend)
{
  runtime.eval(R"(
    (pixils/defcomponent button {:class :ui/control})
    (pixils/defcomponent primary-button
      {:extend 'button
       :class [:ui/primary :ui/cta]})
  )");

  auto& button = get_definition(runtime, "button");
  ASSERT_EQ(button.class_names.size(), 1u);
  EXPECT_EQ(button.class_names[0], "ui/control");

  auto& primary_button = get_definition(runtime, "primary-button");
  ASSERT_EQ(primary_button.class_names.size(), 3u);
  EXPECT_EQ(primary_button.class_names[0], "ui/control");
  EXPECT_EQ(primary_button.class_names[1], "ui/primary");
  EXPECT_EQ(primary_button.class_names[2], "ui/cta");
}

TEST_F(DefModeTest, defmode_focusable_defaults_false_and_can_be_extended_or_overridden)
{
  runtime.eval(R"(
    (pixils/defcomponent button {:focusable true})
    (pixils/defcomponent primary-button {:extend 'button})
    (pixils/defcomponent static-label {:extend 'button :focusable false})
  )");

  auto& button = get_definition(runtime, "button");
  EXPECT_TRUE(button.focusable);

  auto& primary_button = get_definition(runtime, "primary-button");
  EXPECT_TRUE(primary_button.focusable);

  auto& static_label = get_definition(runtime, "static-label");
  EXPECT_FALSE(static_label.focusable);
}

TEST_F(DefModeTest, defmode_extend_preserves_and_overlays_base_style)
{
  runtime.eval(R"(
    (pixils/defcomponent panel
      {:style {:width :fill
               :height :fill
               :clip true}})
    (pixils/defcomponent panel-child
      {:extend 'panel})
    (pixils/defcomponent narrow-panel-child
      {:extend 'panel
       :style {:width 120}})
  )");

  auto& panel_child = get_definition(runtime, "panel-child");
  ASSERT_NE(panel_child.style, std::nullopt);
  ASSERT_NE(panel_child.style->width, std::nullopt);
  ASSERT_NE(panel_child.style->height, std::nullopt);
  ASSERT_NE(panel_child.style->clip, std::nullopt);
  EXPECT_TRUE(panel_child.style->width->is_fill());
  EXPECT_TRUE(panel_child.style->height->is_fill());
  EXPECT_TRUE(*panel_child.style->clip);

  auto& narrow_panel_child = get_definition(runtime, "narrow-panel-child");
  ASSERT_NE(narrow_panel_child.style, std::nullopt);
  ASSERT_NE(narrow_panel_child.style->width, std::nullopt);
  ASSERT_NE(narrow_panel_child.style->height, std::nullopt);
  ASSERT_NE(narrow_panel_child.style->clip, std::nullopt);
  EXPECT_TRUE(narrow_panel_child.style->width->is_fixed());
  EXPECT_EQ(narrow_panel_child.style->width->fixed_value_or(0), 120);
  EXPECT_TRUE(narrow_panel_child.style->height->is_fill());
  EXPECT_TRUE(*narrow_panel_child.style->clip);
}

TEST_F(DefModeTest, defmode_style_accepts_max_size_constraints)
{
  runtime.eval(R"(
    (pixils/defcomponent panel
      {:style {:width :fill
               :height :fill
               :max-width 240
               :max-height 120}})
  )");

  auto& panel = get_definition(runtime, "panel");
  ASSERT_NE(panel.style, std::nullopt);
  ASSERT_NE(panel.style->max_width, std::nullopt);
  ASSERT_NE(panel.style->max_height, std::nullopt);
  EXPECT_EQ(*panel.style->max_width, 240);
  EXPECT_EQ(*panel.style->max_height, 120);
}

TEST_F(DefModeTest, defmode_with_lambda_hook_is_created)
{
  // When
  runtime.eval("(pixils/defmode test-mode {:init (fn [state rc] {:status :initialized})})");

  // Then
  auto mode_val = runtime.eval("(get pixils/modes 'test-mode)");
  Pixils::Runtime::Mode& mode = Roo::obj<Pixils::Runtime::Mode>(*mode_val);
  EXPECT_EQ(mode.init->type, Roo::Value::Type::FUNCTION);
  EXPECT_EQ(*mode.update, *Roo::Constant::NIL);
  EXPECT_EQ(*mode.content_size, *Roo::Constant::NIL);
  EXPECT_EQ(*mode.render, *Roo::Constant::NIL);
}

TEST_F(DefModeTest, defmode_with_content_size_hook_is_created)
{
  // When
  runtime.eval("(pixils/defmode test-mode {:content-size (fn [state ctx] {:w 10 :h 20})})");

  // Then
  auto mode_val = runtime.eval("(get pixils/modes 'test-mode)");
  Pixils::Runtime::Mode& mode = Roo::obj<Pixils::Runtime::Mode>(*mode_val);
  EXPECT_EQ(mode.content_size->type, Roo::Value::Type::FUNCTION);
  EXPECT_EQ(*mode.render, *Roo::Constant::NIL);
}

TEST_F(DefModeTest, defmode_children_get_auto_generated_ids)
{
  // When
  runtime.eval(R"(
    (pixils/defmode child-a {})
    (pixils/defmode child-b {})
    (pixils/defmode parent-mode {:children [{:mode 'child-a} {:mode 'child-b}]})
  )");

  // Then
  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "parent-mode");
  ASSERT_EQ(mode.children.size(), 2u);
  EXPECT_EQ(mode.children[0].id, "child-a-0");
  EXPECT_EQ(mode.children[1].id, "child-b-0");
}

TEST_F(DefModeTest, defmode_two_children_of_same_mode_get_distinct_auto_ids)
{
  // When
  runtime.eval(R"(
    (pixils/defmode panel {})
    (pixils/defmode split-mode {:children [{:mode 'panel} {:mode 'panel}]})
  )");

  // Then
  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "split-mode");
  ASSERT_EQ(mode.children.size(), 2u);
  EXPECT_EQ(mode.children[0].id, "panel-0");
  EXPECT_EQ(mode.children[1].id, "panel-1");
}

TEST_F(DefModeTest, defmode_child_explicit_id_overrides_auto)
{
  // When
  runtime.eval(R"(
    (pixils/defmode child-mode {})
    (pixils/defmode parent-mode {:children [{:mode 'child-mode :id "sidebar"}]})
  )");

  // Then
  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "parent-mode");
  ASSERT_EQ(mode.children.size(), 1u);
  EXPECT_EQ(mode.children[0].id, "sidebar");
}

TEST_F(DefModeTest, string_children_are_normalized_to_text_nodes)
{
  runtime.eval(R"(
    (pixils/defmode parent-mode
      {:children ["Hello" "World"]})
  )");

  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "parent-mode");
  ASSERT_EQ(mode.children.size(), 2u);
  EXPECT_EQ(mode.children[0].component_name, "ui/text");
  EXPECT_TRUE(mode.children[0].mode_name.empty());
  EXPECT_EQ(mode.children[0].id, "ui/text-0");
  auto first_value =
    Roo::Dict::get_property(mode.children[0].initial_state, Roo::keyword("value"));
  ASSERT_NE(first_value, nullptr);
  EXPECT_EQ(first_value->str(), "Hello");

  EXPECT_EQ(mode.children[1].component_name, "ui/text");
  EXPECT_TRUE(mode.children[1].mode_name.empty());
  EXPECT_EQ(mode.children[1].id, "ui/text-1");
  auto second_value =
    Roo::Dict::get_property(mode.children[1].initial_state, Roo::keyword("value"));
  ASSERT_NE(second_value, nullptr);
  EXPECT_EQ(second_value->str(), "World");
}

TEST_F(DefModeTest, raw_string_children_value_is_one_text_node)
{
  runtime.eval(R"(
    (pixils/defmode parent-mode
      {:children "Hello"})
  )");

  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "parent-mode");
  ASSERT_EQ(mode.children.size(), 1u);
  EXPECT_EQ(mode.children[0].component_name, "ui/text");
  EXPECT_TRUE(mode.children[0].mode_name.empty());
  auto value =
    Roo::Dict::get_property(mode.children[0].initial_state, Roo::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->str(), "Hello");
}

TEST_F(DefModeTest, children_reject_non_map_or_string_entries)
{
  EXPECT_THROW(runtime.eval(R"(
    (pixils/defmode parent-mode
      {:children [42]})
  )"),
               Roo::TypeError);
}

TEST_F(DefModeTest, defmode_resources_accept_sounds)
{
  // When
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:resources {:sounds {:laser "laser.wav"}}})
  )");

  // Then
  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "test-mode");
  ASSERT_EQ(mode.resources.sounds.size(), 1u);
  EXPECT_EQ(mode.resources.sounds[0].resource_id, "laser");
  EXPECT_EQ(mode.resources.sounds[0].file_name, "laser.wav");
}

TEST_F(DefModeTest, defmode_resources_accept_music)
{
  // When
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:resources {:music {:theme "theme.mp3"}}})
  )");

  // Then
  Pixils::Runtime::ViewDefinition& mode = get_definition(runtime, "test-mode");
  ASSERT_EQ(mode.resources.music.size(), 1u);
  EXPECT_EQ(mode.resources.music[0].resource_id, "theme");
  EXPECT_EQ(mode.resources.music[0].file_name, "theme.mp3");
}
