
#include "session_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/value.h>

using namespace ::testing;

class SessionHooksTest : public SessionFixture
{
};

TEST_F(SessionHooksTest, push_mode_with_no_hooks_does_not_crash)
{
  // Given
  runtime.eval("(pixils/defmode test-mode {})");

  // Then
  EXPECT_NO_THROW(session.push_mode("test-mode", Lisple::Constant::NIL));
}

TEST_F(SessionHooksTest, push_mode_with_no_init_hook_preserves_initial_state)
{
  // Given
  runtime.eval("(pixils/defmode stateless-mode {})");
  auto initial_state = Lisple::RTValue::number(100);

  // When
  session.push_mode("stateless-mode", initial_state);

  // Then
  EXPECT_TRUE(session.active_mode.state->is_number(100));
}

TEST_F(SessionHooksTest, update_mode_with_no_update_hook_preserves_state)
{
  // Given
  runtime.eval("(pixils/defmode test-mode {})");
  auto initial_state = Lisple::RTValue::number(42);
  session.push_mode("test-mode", initial_state);

  // When
  session.update_mode();

  // Then
  EXPECT_TRUE(session.active_mode.state->is_number(42));
}

TEST_F(SessionHooksTest, update_mode_with_symbol_reference_to_callable_is_invoked)
{
  // Given
  runtime.eval("(defun count-update! [state events ctx] 99)");
  runtime.eval("(pixils/defmode counting-mode {:update count-update!})");
  session.push_mode("counting-mode", Lisple::RTValue::number(0));

  // When
  session.update_mode();

  // Then
  EXPECT_TRUE(session.active_mode.state->is_number(99));
}

TEST_F(SessionHooksTest, update_mode_with_callable_hook_is_invoked)
{
  // Given
  runtime.eval("(pixils/defmode counting-mode {:update (fn [state events ctx] 99)})");
  session.push_mode("counting-mode", Lisple::RTValue::number(0));

  // When
  session.update_mode();

  // Then
  EXPECT_TRUE(session.active_mode.state->is_number(99));
}

TEST_F(SessionHooksTest, push_mode_with_callable_init_hook_is_invoked)
{
  // Given
  runtime.eval("(pixils/defmode init-mode {:init (fn [state ctx] 77)})");

  // When
  session.push_mode("init-mode", Lisple::Constant::NIL);

  // Then
  EXPECT_TRUE(session.active_mode.state->is_number(77));
}

TEST_F(SessionHooksTest, push_mode_with_symbol_init_hook_resolves_and_invokes)
{
  // Given
  runtime.eval("(defun my-init [state ctx] 55)");
  runtime.eval("(pixils/defmode symbol-mode {:init 'test/my-init})");

  // When
  session.push_mode("symbol-mode", Lisple::Constant::NIL);

  // Then
  EXPECT_TRUE(session.active_mode.state->is_number(55));
}
