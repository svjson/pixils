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
}

TEST_F(DefModeTest, defmode_with_no_args_is_created_with_nil_hooks)
{
  // When
  runtime.eval("(pixils/defmode test-mode {})");

  // Then
  auto mode_val = runtime.eval("(get pixils/modes 'test-mode)");
  Pixils::Runtime::Mode& mode = Lisple::obj<Pixils::Runtime::Mode>(*mode_val);
  EXPECT_EQ(*mode.init, *Lisple::Constant::NIL);
  EXPECT_EQ(*mode.update, *Lisple::Constant::NIL);
  EXPECT_EQ(*mode.render, *Lisple::Constant::NIL);
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
  EXPECT_EQ(*mode.render, *Lisple::Constant::NIL);
}

TEST_F(DefModeTest, defmode_children_get_auto_generated_ids)
{
  // When
  runtime.eval(R"(
    (pixils/defmode child-a {})
    (pixils/defmode child-b {})
    (pixils/defmode parent-mode {:children [{:mode child-a} {:mode child-b}]})
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
    (pixils/defmode split-mode {:children [{:mode panel} {:mode panel}]})
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
    (pixils/defmode parent-mode {:children [{:mode child-mode :id "sidebar"}]})
  )");

  // Then
  Pixils::Runtime::Mode& mode = get_mode(runtime, "parent-mode");
  ASSERT_EQ(mode.children.size(), 1u);
  EXPECT_EQ(mode.children[0].id, "sidebar");
}
