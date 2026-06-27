
#include "session_fixture.h"

#include <SDL2/SDL_keycode.h>
#include <gtest/gtest.h>
#include <roo/runtime/value.h>

using namespace ::testing;

class SessionHooksTest : public SessionFixture
{
};

TEST_F(SessionHooksTest, push_mode_with_no_hooks_does_not_crash)
{
  // Given
  runtime.eval("(pixils/defmode test-mode {})");

  // Then
  EXPECT_NO_THROW(session.push_mode("test-mode", Roo::Constant::NIL));
}

TEST_F(SessionHooksTest, push_mode_with_no_init_hook_preserves_initial_state)
{
  // Given
  runtime.eval("(pixils/defmode stateless-mode {})");
  auto initial_state = Roo::number(100);

  // When
  session.push_mode("stateless-mode", initial_state);

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(100));
}

TEST_F(SessionHooksTest, update_mode_with_no_update_hook_preserves_state)
{
  // Given
  runtime.eval("(pixils/defmode test-mode {})");
  auto initial_state = Roo::number(42);
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
  session.push_mode("counting-mode", Roo::number(0));

  // When
  session.update_mode();

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(99));
}

TEST_F(SessionHooksTest, update_mode_with_callable_hook_is_invoked)
{
  // Given
  runtime.eval("(pixils/defmode counting-mode {:update (fn [state ctx] 99)})");
  session.push_mode("counting-mode", Roo::number(0));

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
  session.push_mode("init-mode", Roo::Constant::NIL);

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(77));
}

TEST_F(SessionHooksTest, push_mode_with_symbol_init_hook_resolves_and_invokes)
{
  // Given
  runtime.eval("(defun my-init [state ctx] 55)");
  runtime.eval("(pixils/defmode symbol-mode {:init 'test/my-init})");

  // When
  session.push_mode("symbol-mode", Roo::Constant::NIL);

  // Then
  EXPECT_TRUE(session.active_mode->state->is_number(55));
}

TEST_F(SessionHooksTest, push_mode_with_symbol_content_size_hook_resolves)
{
  // Given
  runtime.eval("(defun my-content-size [state ctx] {:w 10 :h 20})");
  runtime.eval("(pixils/defmode symbol-mode {:content-size 'test/my-content-size})");

  // When
  session.push_mode("symbol-mode", Roo::Constant::NIL);

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_NE(session.active_mode->mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->content_size->type, Roo::Value::Type::FUNCTION);
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
  session.push_mode("key-mode", Roo::Constant::NIL);

  // When
  input().key_down(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:count 1 :last-key :key/space}");
}

TEST_F(SessionHooksTest, key_down_push_pop_does_not_replay_into_new_active_mode)
{
  runtime.eval(R"(
    (pixils/defmode inventory-mode
      {:on-key-down (fn [state event ctx]
                      (if (= (:key event) :key/I)
                        (do
                          (pixils/pop-mode!)
                          state)
                        state))})

    (pixils/defmode root-mode
      {:on-key-down (fn [state event ctx]
                      (if (= (:key event) :key/I)
                        (do
                          (pixils/push-mode! 'inventory-mode)
                          state)
                        state))})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  input().key_down(SDLK_i);
  update_cycle();
  ASSERT_EQ(session.active_mode->mode->name, "inventory-mode");

  update_cycle();
  EXPECT_EQ(session.active_mode->mode->name, "inventory-mode");

  input().key_up(SDLK_i);
  update_cycle();

  input().key_down(SDLK_i);
  update_cycle();
  ASSERT_EQ(session.active_mode->mode->name, "root-mode");

  update_cycle();
  EXPECT_EQ(session.active_mode->mode->name, "root-mode");
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
  session.push_mode("key-mode", Roo::Constant::NIL);

  // When
  input().key_down(SDLK_SPACE);
  input().clear_transients();
  input().key_up(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:count 1 :last-key :key/space}");
}

TEST_F(SessionHooksTest, root_mode_action_map_emits_matching_action_event)
{
  runtime.eval(R"(
    (pixils/defmode action-mode
      {:init (fn [state ctx] {:count 0 :last-payload nil})
       :action-map {:key/f2 {:action :game/new-game
                             :payload {:source :action-map}}}
       :on {:game/new-game (fn [state event ctx]
                             (assoc (assoc state :count (+ (:count state) 1))
                                    :last-payload (:payload event)))}})
  )");

  session.push_mode("action-mode", Roo::Constant::NIL);

  input().key_down(SDLK_F2);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(),
            "{:count 1 :last-payload {:source :action-map}}");
}

TEST_F(SessionHooksTest, focused_child_stopping_key_down_prevents_action_map_dispatch)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:focusable true
       :init (fn [state ctx] {:keys 0})
       :on-key-down (fn [state event ctx]
                      (do (pixils.ui/stop-propagation! event)
                          (assoc state :keys (+ (:keys state) 1))))})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:child {:keys 0}
                              :actions 0})
       :action-map {:key/space :root/action}
       :on {:root/action (fn [state event ctx]
                           (assoc state :actions (+ (:actions state) 1)))}
       :children [{:mode 'child-mode
                   :id "child"
                   :state (pixils.ui/bind-state :child)}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  session.update_mode();
  input().clear_transients();

  input().key_down(SDLK_SPACE);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:child {:keys 1} :actions 0}");
}

TEST_F(SessionHooksTest, root_mode_action_map_matches_modifier_vector_shortcuts)
{
  runtime.eval(R"(
    (pixils/defmode action-mode
      {:init (fn [state ctx] {:tag :none})
       :action-map {[:key/shift :key/f5] :help/about}
       :on {:help/about (fn [state event ctx]
                          (assoc state :tag :matched))}})
  )");

  session.push_mode("action-mode", Roo::Constant::NIL);

  input().key_down(SDLK_LSHIFT);
  input().clear_transients();
  input().key_down(SDLK_F5);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:tag :matched}");
}

TEST_F(SessionHooksTest, ui_children_returns_child_views_for_view_and_hook_context)
{
  runtime.eval(R"(
    (pixils/defmode child-mode {})
    (pixils/defmode parent-mode
      {:init (fn [state ctx] {:from-ctx nil :from-view nil :first-child-id nil})
       :update (fn [state ctx]
                 (let [children-from-ctx (pixils.ui/children ctx)
                       children-from-view (pixils.ui/children (:view ctx))
                       first-child (head children-from-ctx)]
                   {:from-ctx (count children-from-ctx)
                    :from-view (count children-from-view)
                    :first-child-id (:id first-child)}))
       :children [{:mode 'child-mode}]})
  )");

  session.push_mode("parent-mode", Roo::Constant::NIL);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(),
            "{:from-ctx 1 :from-view 1 :first-child-id \"child-mode-0\"}");
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
  session.push_mode("key-mode", Roo::Constant::NIL);

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
  session.push_mode("key-mode", Roo::Constant::NIL);

  // When
  input().key_down(SDLK_LCTRL);
  input().clear_transients();
  input().key_down(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:tag :combo}");
}

TEST_F(SessionHooksTest, keyboard_event_to_text_appends_printable_text_in_key_down_hook)
{
  runtime.eval(R"(
    (pixils/defmode key-mode
      {:init (fn [state ctx] {:value ""})
       :on-key-down (fn [state event ctx]
                      (if-let [text (pixils.keyboard/event->text event)]
                        (assoc state :value (str (:value state) text))
                        state))})
  )");
  session.push_mode("key-mode", Roo::Constant::NIL);

  input().key_down(SDLK_a);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:value \"a\"}");
}

TEST_F(SessionHooksTest, keyboard_event_to_text_handles_shifted_and_punctuation_keys)
{
  runtime.eval(R"(
    (pixils/defmode key-mode
      {:init (fn [state ctx] {:value ""})
       :on-key-down (fn [state event ctx]
                      (if-let [text (pixils.keyboard/event->text event)]
                        (assoc state :value (str (:value state) text))
                        state))})
  )");
  session.push_mode("key-mode", Roo::Constant::NIL);

  input().key_down(SDLK_LSHIFT);
  input().clear_transients();
  input().key_down(SDLK_1);
  session.update_mode();
  EXPECT_EQ(session.active_mode->state->to_string(), "{:value \"!\"}");

  input().clear_transients();
  input().key_up(SDLK_1);
  input().key_up(SDLK_LSHIFT);
  session.update_mode();
  input().clear_transients();

  input().key_down(SDLK_COMMA);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:value \"!,\"}");
}

TEST_F(SessionHooksTest, keyboard_event_to_text_handles_mode_key_as_altgr)
{
  runtime.eval(R"(
    (pixils/defmode key-mode
      {:init (fn [state ctx] {:value ""})
       :on-key-down (fn [state event ctx]
                      (if-let [text (pixils.keyboard/event->text event)]
                        (assoc state :value (str (:value state) text))
                        state))})
  )");
  session.push_mode("key-mode", Roo::Constant::NIL);

  input().key_down(SDLK_MODE);
  input().clear_transients();
  input().key_down(SDLK_7);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:value \"{\"}");
}

TEST_F(SessionHooksTest, focused_child_mode_on_key_down_bubbles_to_root_mode)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:focusable true
       :init (fn [state ctx] {:keys 0})
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
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  session.update_mode();
  input().clear_transients();

  // When
  input().key_down(SDLK_SPACE);
  session.update_mode();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:child {:keys 1} :root-keys 1}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(), "{:keys 1}");
}

TEST_F(SessionHooksTest, child_mode_focused_in_init_receives_key_down)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:focusable true
       :init (fn [state ctx]
               (do (pixils.ui/focus! ctx)
                   {:keys 0}))
       :on-key-down (fn [state event ctx]
                      (assoc state :keys (+ (:keys state) 1)))})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:child {:keys 0}})
       :children [{:mode 'child-mode
                   :id "child"
                   :state (pixils.ui/bind-state :child)}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();

  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.focus_state.focused.lock().get(),
            session.active_mode->children[0].get());

  input().key_down(SDLK_SPACE);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:child {:keys 1}}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(), "{:keys 1}");
}

TEST_F(SessionHooksTest, focused_child_key_stop_propagation_prevents_root_key_hook)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:focusable true
       :init (fn [state ctx] {:keys 0})
       :on-key-down (fn [state event ctx]
                      (do (pixils.ui/stop-propagation! event)
                          (assoc state :keys (+ (:keys state) 1))))})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:child {:keys 0}
                              :root-keys 0})
       :on-key-down (fn [state event ctx]
                      (assoc state :root-keys (+ (:root-keys state) 1)))
       :children [{:mode 'child-mode
                   :id "child"
                   :state (pixils.ui/bind-state :child)}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  session.update_mode();
  input().clear_transients();

  input().key_down(SDLK_SPACE);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:child {:keys 1} :root-keys 0}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(), "{:keys 1}");
}

TEST_F(SessionHooksTest, focused_child_mode_on_key_held_bubbles_to_root_mode)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:focusable true
       :init (fn [state ctx] {:tag :none})
       :on-key-held {[:key/left-ctrl :key/space] (fn [state event ctx]
                                                   (assoc state :tag :combo))}})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:child {:tag :none}
                              :root-held 0})
       :on-key-held {[:key/left-ctrl :key/space] (fn [state event ctx]
                                                   (assoc state :root-held
                                                                (+ (:root-held state) 1)))}
       :children [{:mode 'child-mode
                   :id "child"
                   :state (pixils.ui/bind-state :child)}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  session.update_mode();
  EXPECT_EQ(session.active_mode->state->to_string(), "{:child {:tag :none} :root-held 0}");
  input().clear_transients();

  input().key_down(SDLK_LCTRL);
  input().clear_transients();
  input().key_down(SDLK_SPACE);
  session.update_mode();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:child {:tag :combo} :root-held 1}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(), "{:tag :combo}");
}
