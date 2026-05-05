
#include "session_fixture.h"

#include <SDL2/SDL_keycode.h>
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
  EXPECT_TRUE(session.active_mode->state->is_number(100));
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
  EXPECT_TRUE(session.active_mode->state->is_number(42));
}

TEST_F(SessionHooksTest, update_mode_with_symbol_reference_to_callable_is_invoked)
{
  // Given
  runtime.eval("(defun count-update! [state ctx] 99)");
  runtime.eval("(pixils/defmode counting-mode {:update count-update!})");
  session.push_mode("counting-mode", Lisple::RTValue::number(0));

  // When
  session.update_mode();

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(99));
}

TEST_F(SessionHooksTest, update_mode_with_callable_hook_is_invoked)
{
  // Given
  runtime.eval("(pixils/defmode counting-mode {:update (fn [state ctx] 99)})");
  session.push_mode("counting-mode", Lisple::RTValue::number(0));

  // When
  session.update_mode();

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(99));
}

TEST_F(SessionHooksTest, push_mode_with_callable_init_hook_is_invoked)
{
  // Given
  runtime.eval("(pixils/defmode init-mode {:init (fn [state ctx] 77)})");

  // When
  session.push_mode("init-mode", Lisple::Constant::NIL);

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(77));
}

TEST_F(SessionHooksTest, push_mode_with_symbol_init_hook_resolves_and_invokes)
{
  // Given
  runtime.eval("(defun my-init [state ctx] 55)");
  runtime.eval("(pixils/defmode symbol-mode {:init 'test/my-init})");

  // When
  session.push_mode("symbol-mode", Lisple::Constant::NIL);

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(55));
}

TEST_F(SessionHooksTest, push_mode_with_symbol_content_size_hook_resolves)
{
  // Given
  runtime.eval("(defun my-content-size [state ctx] {:w 10 :h 20})");
  runtime.eval("(pixils/defmode symbol-mode {:content-size 'test/my-content-size})");

  // When
  session.push_mode("symbol-mode", Lisple::Constant::NIL);

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_NE(session.active_mode->mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->content_size->type, Lisple::RTValue::Type::FUNCTION);
}

TEST_F(SessionHooksTest, root_mode_on_key_down_hook_is_invoked)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode key-mode
      {:init (fn [state ctx] {:count 0 :last-key nil})
       :on-key-down (fn [state event ctx]
                      (assoc (assoc state :last-key (:key event))
                             :count (+ (:count state) 1)))})
  )");
  session.push_mode("key-mode", Lisple::Constant::NIL);

  // When
  input().key_down(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:count 1 :last-key :key/space}");
}

TEST_F(SessionHooksTest, root_mode_on_key_up_hook_is_invoked)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode key-mode
      {:init (fn [state ctx] {:count 0 :last-key nil})
       :on-key-up (fn [state event ctx]
                    (assoc (assoc state :last-key (:key event))
                           :count (+ (:count state) 1)))})
  )");
  session.push_mode("key-mode", Lisple::Constant::NIL);

  // When
  input().key_down(SDLK_SPACE);
  input().clear_transients();
  input().key_up(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:count 1 :last-key :key/space}");
}

TEST_F(SessionHooksTest, root_mode_on_key_held_function_is_invoked_once_per_frame)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode key-mode
      {:init (fn [state ctx] {:count 0 :held-count 0})
       :on-key-held (fn [state event ctx]
                      (assoc (assoc state :held-count (count (:held-keys event)))
                             :count (+ (:count state) 1)))})
  )");
  session.push_mode("key-mode", Lisple::Constant::NIL);

  // When
  input().key_down(SDLK_LCTRL);
  input().clear_transients();
  input().key_down(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:count 1 :held-count 2}");
}

TEST_F(SessionHooksTest, root_mode_on_key_held_map_prefers_more_specific_combo_match)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode key-mode
      {:init (fn [state ctx] {:tag :none})
       :on-key-held {:key/left-ctrl (fn [state event ctx]
                                      (assoc state :tag :single))
                     [:key/left-ctrl :key/space] (fn [state event ctx]
                                                   (assoc state :tag :combo))}})
  )");
  session.push_mode("key-mode", Lisple::Constant::NIL);

  // When
  input().key_down(SDLK_LCTRL);
  input().clear_transients();
  input().key_down(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:tag :combo}");
}

TEST_F(SessionHooksTest, child_mode_key_hooks_do_not_fire_without_focus_routing)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:init (fn [state ctx] {:keys 0})
       :on-key-down (fn [state event ctx]
                      (assoc state :keys (+ (:keys state) 1)))})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:child {:keys 0}
                              :root-keys 0})
       :on-key-down (fn [state event ctx]
                      (assoc state :root-keys (+ (:root-keys state) 1)))
       :children [{:mode 'child-mode
                   :id "child"
                   :state (pixils.ui/bind-state :child)}]})
  )");
  session.push_mode("root-mode", Lisple::Constant::NIL);

  // When
  input().key_down(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:child {:keys 0} :root-keys 1}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(), "{:keys 0}");
}
