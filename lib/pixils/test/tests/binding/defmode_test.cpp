#include "../fixture.h"
#include "pixils/runtime/mode.h"

#include <gtest/gtest.h>
#include <lisple/runtime/value.h>

using DefModeTest = BaseFixture;

namespace
{
  Pixils::Runtime::Mode& get_mode(Lisple::Runtime& rt, const std::string& name)
  {
    auto val = rt.eval("(get pixils/modes '" + name + ")");
    return Lisple::obj<Pixils::Runtime::Mode>(*val);
  }
} // namespace

TEST_F(DefModeTest, defmode_with_no_args_is_created_with_nil_hooks)
{
  // When
  runtime.eval("(pixils/defmode test-mode {})");

  // Then
  auto mode_val = runtime.eval("(get pixils/modes 'test-mode)");
  Pixils::Runtime::Mode& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);
  ASSERT_EQ(mode.selector_modes.size(), 1u);
  EXPECT_EQ(mode.selector_modes[0], "test-mode");
  EXPECT_EQ(*mode.init, *Lisple::Constant::NIL);
  EXPECT_EQ(*mode.update, *Lisple::Constant::NIL);
  EXPECT_EQ(*mode.content_size, *Lisple::Constant::NIL);
  EXPECT_EQ(*mode.render, *Lisple::Constant::NIL);
}

TEST_F(DefModeTest, defmode_extend_preserves_selector_ancestry_for_theme_matching)
{
  runtime.eval(R"(
    (pixils/defcomponent button {})
    (pixils/defcomponent board-button {:extend 'button})
    (pixils/defcomponent special-board-button {:extend 'board-button})
  )");

  auto& board_button = get_mode(runtime, "board-button");
  ASSERT_EQ(board_button.selector_modes.size(), 2u);
  EXPECT_EQ(board_button.selector_modes[0], "board-button");
  EXPECT_EQ(board_button.selector_modes[1], "button");

  auto& special_board_button = get_mode(runtime, "special-board-button");
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

  auto& button = get_mode(runtime, "button");
  ASSERT_EQ(button.class_names.size(), 1u);
  EXPECT_EQ(button.class_names[0], "ui/control");

  auto& primary_button = get_mode(runtime, "primary-button");
  ASSERT_EQ(primary_button.class_names.size(), 3u);
  EXPECT_EQ(primary_button.class_names[0], "ui/control");
  EXPECT_EQ(primary_button.class_names[1], "ui/primary");
  EXPECT_EQ(primary_button.class_names[2], "ui/cta");
}

TEST_F(DefModeTest, defmode_with_lambda_hook_is_created)
{
  // When
  runtime.eval("(pixils/defmode test-mode {:init (fn [state rc] {:status :initialized})})");

  // Then
  auto mode_val = runtime.eval("(get pixils/modes 'test-mode)");
  Pixils::Runtime::Mode& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);
  EXPECT_EQ(mode.init->type, Lisple::RTValue::Type::FUNCTION);
  EXPECT_EQ(*mode.update, *Lisple::Constant::NIL);
  EXPECT_EQ(*mode.content_size, *Lisple::Constant::NIL);
  EXPECT_EQ(*mode.render, *Lisple::Constant::NIL);
}

TEST_F(DefModeTest, defmode_with_content_size_hook_is_created)
{
  // When
  runtime.eval("(pixils/defmode test-mode {:content-size (fn [state ctx] {:w 10 :h 20})})");

  // Then
  auto mode_val = runtime.eval("(get pixils/modes 'test-mode)");
  Pixils::Runtime::Mode& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);
  EXPECT_EQ(mode.content_size->type, Lisple::RTValue::Type::FUNCTION);
  EXPECT_EQ(*mode.render, *Lisple::Constant::NIL);
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
  Pixils::Runtime::Mode& mode = get_mode(runtime, "parent-mode");
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
  Pixils::Runtime::Mode& mode = get_mode(runtime, "split-mode");
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
  Pixils::Runtime::Mode& mode = get_mode(runtime, "parent-mode");
  ASSERT_EQ(mode.children.size(), 1u);
  EXPECT_EQ(mode.children[0].id, "sidebar");
}

TEST_F(DefModeTest, defmode_resources_accept_sounds)
{
  // When
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:resources {:sounds {:laser "laser.wav"}}})
  )");

  // Then
  Pixils::Runtime::Mode& mode = get_mode(runtime, "test-mode");
  ASSERT_EQ(mode.resources.sounds.size(), 1u);
  EXPECT_EQ(mode.resources.sounds[0].resource_id, "laser");
  EXPECT_EQ(mode.resources.sounds[0].file_name, "laser.wav");
}
