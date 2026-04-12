#include "../fixture.h"
#include "pixils/runtime/mode.h"

#include <gtest/gtest.h>
#include <lisple/runtime/value.h>

using DefModeTest = BaseFixture;

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
