
#include "session_fixture.h"
#include <pixils/ui/style.h>

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

using EventRoutingTest = SessionFixture;

TEST_F(EventRoutingTest, on_click_fires_when_mouse_down_and_up_on_same_view)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode clickable {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
  )");
  session.push_mode("clickable", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  // When - mouse-down at (50,50)
  input().mouse_down({50, 50});
  update_cycle();

  // When - mouse-up at same position
  input().mouse_up({50, 50});
  update_cycle();

  // Then
  EXPECT_EQ(session.active_mode->state->to_string(), "{:clicks 1}");
}

TEST_F(EventRoutingTest, on_double_click_fires_for_second_click)
{
  runtime.eval(R"(
    (pixils/defmode clickable {
      :init (fn [state ctx] {:clicks 0 :double-clicks 0 :click-count 0})
      :on-click (fn [state ev ctx]
                  (assoc state :clicks (+ (:clicks state) 1)))
      :on-double-click (fn [state ev ctx]
                         (assoc state
                                :double-clicks (+ (:double-clicks state) 1)
                                :click-count (:click-count ev)))
    })
  )");
  session.push_mode("clickable", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  input().mouse_down({50, 50});
  update_cycle();
  input().mouse_up({50, 50});
  update_cycle();

  input().mouse_down({50, 50}, SDL_BUTTON_LEFT, 2);
  update_cycle();
  input().mouse_up({50, 50}, SDL_BUTTON_LEFT, 2);
  update_cycle();

  EXPECT_EQ(session.active_mode->state->to_string(),
            "{:clicks 2 :double-clicks 1 :click-count 2}");
}

TEST_F(EventRoutingTest, clipped_view_does_not_hit_children_outside_content_rect)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:init (fn [state ctx] {:clicks 0})
       :on-click (fn [state event ctx]
                   (assoc state :clicks (+ (:clicks state) 1)))})

    (pixils/defmode root-mode
      {:style {:width 100 :height 100 :padding 20 :clip true}
       :children [{:mode 'child-mode
                   :style {:position :absolute
                           :left 0
                           :top 0
                           :width 100
                           :height 100}}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();

  input().mouse_down({10, 10});
  update_cycle();
  input().mouse_up({10, 10});
  update_cycle();

  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->state->to_string(), "{:clicks 0}");
}

TEST_F(EventRoutingTest, on_click_does_not_fire_when_mouse_up_on_different_view)
{
  // Given - two side-by-side sibling views
  runtime.eval(R"(
    (pixils/defmode btn-left {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
    (pixils/defmode btn-right {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
    (pixils/defmode split-view {:children [{:mode 'btn-left :id "left"}
                                           {:mode 'btn-right :id "right"}]})
  )");
  session.push_mode("split-view", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 100};
  session.active_mode->children[0]->bounds = {0, 0, 100, 100};
  session.active_mode->children[1]->bounds = {100, 0, 100, 100};

  // When - press on left button
  input().mouse_down({25, 50});
  update_cycle();

  /**
   * Move the cursor to the right view in a separate frame so that
   * mouse.hovered is updated before handle_mouse_up runs. This matches
   * real usage: SDL delivers motion and button events in separate frames.
   */
  input().mouse_move({150, 50});
  update_cycle();

  // When - release on right button
  input().mouse_up({150, 50});
  update_cycle();

  // Then - neither button received a click
  auto& left_view = *session.active_mode->children[0];
  EXPECT_EQ(left_view.state->to_string(), "{:clicks 0}");

  auto& right_view = *session.active_mode->children[1];
  EXPECT_EQ(right_view.state->to_string(), "{:clicks 0}");
}

TEST_F(EventRoutingTest, on_mouse_up_fires_on_hovered_view_regardless_of_press_origin)
{
  // Given - two side-by-side sibling views; only right tracks on-mouse-up
  runtime.eval(R"(
    (pixils/defmode btn-left {})
    (pixils/defmode btn-right {
      :init        (fn [state ctx] {:up-count 0})
      :on-mouse-up (fn [state ev ctx] (assoc state :up-count (+ (:up-count state) 1)))
    })
    (pixils/defmode split-view {:children [{:mode 'btn-left :id "left"}
                                           {:mode 'btn-right :id "right"}]})
  )");
  session.push_mode("split-view", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 100};
  session.active_mode->children[0]->bounds = {0, 0, 100, 100};
  session.active_mode->children[1]->bounds = {100, 0, 100, 100};

  // When - press on left, move to right (separate frame), then release
  input().mouse_down({25, 50});
  update_cycle();

  input().mouse_move({150, 50});
  update_cycle();

  input().mouse_up({150, 50});
  update_cycle();

  // Then - right button received on-mouse-up even though press started on left
  auto& left_view = *session.active_mode->children[0];
  EXPECT_EQ(left_view.state->to_string(), "nil");

  auto& right_view = *session.active_mode->children[1];
  EXPECT_EQ(right_view.state->to_string(), "{:up-count 1}");
}

TEST_F(EventRoutingTest, child_click_state_propagates_into_parent_state_map)
{
  // Given - parent with a clickable child
  runtime.eval(R"(
    (pixils/defmode btn-mode {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
    (pixils/defmode container-mode {:children [{:mode 'btn-mode :id "btn"}]})
  )");
  session.push_mode("container-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  // When - click on the child button
  input().mouse_down({50, 50});
  update_cycle();

  input().mouse_up({50, 50});
  update_cycle();

  // Then - updated child state is visible in the parent's state map
  auto& btn_mode = *session.active_mode->children[0];
  EXPECT_EQ(btn_mode.state->to_string(), "{:clicks 1}");
}

TEST_F(EventRoutingTest, child_mouse_down_state_propagates_into_parent_state_map)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode btn-mode {
      :init          (fn [state ctx] {:pressed-count 0})
      :on-mouse-down (fn [state ev ctx]
                       (assoc state :pressed-count (+ (:pressed-count state) 1)))
    })
    (pixils/defmode container-mode {:children [{:mode 'btn-mode :id "btn"}]})
  )");
  session.push_mode("container-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  // When
  input().mouse_down({50, 50});
  update_cycle();

  // Then
  auto& btn_mode = *session.active_mode->children[0];
  EXPECT_EQ(btn_mode.state->to_string(), "{:pressed-count 1}");
}

TEST_F(EventRoutingTest, on_mouse_enter_state_change_propagates_to_parent)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode hoverable {
      :init            (fn [state ctx] {:entered 0})
      :on-mouse-enter  (fn [state ev ctx] (assoc state :entered (+ (:entered state) 1)))
    })
    (pixils/defmode container-mode {:children [{:mode 'hoverable :id "panel"}]})
  )");
  session.push_mode("container-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  // When - move mouse outside child first, then into child
  input().mouse_move({5, 5});
  update_cycle();

  input().mouse_move({50, 50});
  update_cycle();

  // Then
  auto& panel_mode = *session.active_mode->children[0];
  EXPECT_EQ(panel_mode.state->to_string(), "{:entered 1}");
}

TEST_F(EventRoutingTest, on_mouse_leave_state_change_propagates_to_parent)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode hoverable {
      :init           (fn [state ctx] {:left-count 0})
      :on-mouse-leave (fn [state ev ctx] (assoc state :left-count (+ (:left-count state) 1)))
    })
    (pixils/defmode container-mode {:children [{:mode 'hoverable :id "panel"}]})
  )");
  session.push_mode("container-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  // When - enter child, then leave it
  input().mouse_move({50, 50});
  update_cycle();

  input().mouse_move({5, 5});
  update_cycle();

  // Then
  auto& panel_mode = *session.active_mode->children[0];
  EXPECT_EQ(panel_mode.state->to_string(), "{:left-count 1}");
}

TEST_F(EventRoutingTest, drag_hooks_fire_on_pressed_view_chain_after_motion)
{
  runtime.eval(R"(
    (pixils/defmode titlebar {
      :init          (fn [state ctx] {:drag-starts 0 :last-drag nil})
      :on-drag-start (fn [state event ctx]
                       (assoc state :drag-starts (+ (:drag-starts state) 1)))
      :on-drag       (fn [state event ctx]
                       (assoc state :last-drag (:total-delta event)))
    })
  )");
  session.push_mode("titlebar", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 20};

  input().mouse_down({10, 10});
  update_cycle();

  input().mouse_move({18, 14});
  update_cycle();

  input().mouse_move({22, 16});
  update_cycle();

  EXPECT_EQ(session.active_mode->state->to_string(),
            "{:drag-starts 1 :last-drag {:x 12 :y 6}}");
}

TEST_F(EventRoutingTest, drag_continues_after_cursor_leaves_pressed_view)
{
  runtime.eval(R"(
    (pixils/defmode draggable {
      :init    (fn [state ctx] {:last nil})
      :on-drag (fn [state event ctx]
                 (assoc state :last (:total-delta event)))
    })
  )");
  session.push_mode("draggable", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 40, 20};

  input().mouse_down({10, 10});
  update_cycle();

  input().mouse_move({20, 12});
  update_cycle();

  input().mouse_move({80, 12});
  update_cycle();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:last {:x 70 :y 2}}");
}

TEST_F(EventRoutingTest, drag_policy_payload_is_delivered_to_drop_target)
{
  runtime.eval(R"(
    (pixils/defmode drag-source {
      :init (fn [state ctx] {:id :disk :started nil})
      :drag {:start {:mode :threshold :distance 8}
             :payload (fn [state event ctx]
                        {:kind :icon
                         :id (:id state)
                         :grab (:start-position event)})}
      :on-drag-start (fn [state event ctx]
                       (assoc state :started (:payload event)))
    })
    (pixils/defmode drop-target {
      :init    (fn [state ctx] {:dropped nil})
      :on-drop (fn [state event ctx]
                 (assoc state :dropped (:payload event)))
    })
    (pixils/defmode split-view {:children [{:mode 'drag-source :id "source"}
                                           {:mode 'drop-target :id "target"}]})
  )");
  session.push_mode("split-view", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 120, 40};
  session.active_mode->children[0]->bounds = {0, 0, 50, 40};
  session.active_mode->children[1]->bounds = {60, 0, 50, 40};

  input().mouse_down({10, 10});
  update_cycle();

  input().mouse_move({14, 10});
  update_cycle();
  EXPECT_EQ(session.active_mode->children[0]->state->to_string(),
            "{:id :disk :started nil}");

  input().mouse_move({70, 10});
  update_cycle();
  input().mouse_up({70, 10});
  update_cycle();

  EXPECT_EQ(session.active_mode->children[0]->state->to_string(),
            "{:id :disk :started {:kind :icon :id :disk :grab {:x 10 :y 10}}}");
  EXPECT_EQ(session.active_mode->children[1]->state->to_string(),
            "{:dropped {:kind :icon :id :disk :grab {:x 10 :y 10}}}");
}

TEST_F(EventRoutingTest, drag_end_suppresses_click_and_updates_state)
{
  runtime.eval(R"(
    (pixils/defmode draggable {
      :init          (fn [state ctx] {:clicks 0 :drag-ended nil})
      :on-click      (fn [state event ctx]
                       (assoc state :clicks (+ (:clicks state) 1)))
      :on-drag-start (fn [state event ctx] state)
      :on-drag-end   (fn [state event ctx]
                       (assoc state :drag-ended (:total-delta event)))
    })
  )");
  session.push_mode("draggable", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 20};

  input().mouse_down({10, 10});
  update_cycle();

  input().mouse_move({30, 10});
  update_cycle();

  input().mouse_up({30, 10});
  update_cycle();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:clicks 0 :drag-ended {:x 20 :y 0}}");
}

TEST_F(EventRoutingTest, child_drag_state_propagates_into_parent_state_map)
{
  runtime.eval(R"(
    (pixils/defmode titlebar {
      :init    (fn [state ctx] {:last nil})
      :on-drag-start (fn [state event ctx]
                       (assoc state :last (:total-delta event)))
    })
    (pixils/defmode window-mode {
      :init (fn [state ctx] {:titlebar {:last nil}})
      :children [{:mode 'titlebar :id "titlebar"
                  :state (pixils.ui/bind-state :titlebar)}]
    })
  )");
  session.push_mode("window-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 120, 80};
  session.active_mode->children[0]->bounds = {0, 0, 120, 20};

  input().mouse_down({10, 10});
  update_cycle();

  input().mouse_move({25, 18});
  update_cycle();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:titlebar {:last {:x 15 :y 8}}}");
}

TEST_F(EventRoutingTest, interaction_emitted_event_state_propagates_through_bound_ancestors)
{
  runtime.eval(R"(
    (pixils/defmode drag-source {
      :drag {:start {:mode :threshold :distance 3}
             :payload (fn [state event ctx] nil)}
      :on-drag-start (fn [state event ctx]
                       (do
                         (pixils.ui/emit! (:view ctx) :source/dragged nil)
                         state))
    })
    (pixils/defmode mid-mode {
      :on {:source/dragged (fn [state event ctx]
                             (assoc state :count (+ (:count state) 1)))}
      :children [{:mode 'drag-source :id "source"}]
    })
    (pixils/defmode outer-mode {
      :children [{:mode 'mid-mode :id "mid"
                  :state (pixils.ui/bind-state :mid)}]
    })
    (pixils/defmode root-mode {
      :init (fn [state ctx] {:outer {:mid {:count 0}}})
      :children [{:mode 'outer-mode :id "outer"
                  :state (pixils.ui/bind-state :outer)}]
    })
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 40};
  session.active_mode->children[0]->bounds = {0, 0, 100, 40};
  session.active_mode->children[0]->children[0]->bounds = {0, 0, 100, 40};
  session.active_mode->children[0]->children[0]->children[0]->bounds = {0, 0, 100, 40};

  input().mouse_down({10, 10});
  update_cycle();
  input().mouse_move({20, 10});
  update_cycle();

  EXPECT_EQ(session.active_mode->state->to_string(), "{:outer {:mid {:count 1}}}");
}

TEST_F(EventRoutingTest, stop_propagation_accepts_drag_events)
{
  runtime.eval(R"(
    (pixils/defmode child-mode {
      :on-drag-start (fn [state event ctx]
                       (do
                         (pixils.ui/stop-propagation! event)
                         (assoc state :child-starts (+ (:child-starts state) 1))))
    })
    (pixils/defmode parent-mode {
      :init (fn [state ctx] {:parent-starts 0 :child {:child-starts 0}})
      :on-drag-start (fn [state event ctx]
                       (assoc state :parent-starts (+ (:parent-starts state) 1)))
      :children [{:mode 'child-mode :id "child"
                  :state (pixils.ui/bind-state :child)}]
    })
  )");
  session.push_mode("parent-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 40};
  session.active_mode->children[0]->bounds = {0, 0, 100, 40};

  input().mouse_down({10, 10});
  update_cycle();

  input().mouse_move({20, 10});
  update_cycle();

  EXPECT_EQ(session.active_mode->state->to_string(),
            "{:parent-starts 0 :child {:child-starts 1}}");
}

TEST_F(EventRoutingTest, nested_child_events_update_bound_ancestor_state_during_traverse)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode emitter-mode {
      :update (fn [state ctx]
                (do (pixils.ui/emit! (:view ctx) :ping nil)
                    state))
    })
    (pixils/defmode mid-mode {
      :on {:ping (fn [state event ctx]
                   (assoc state :count (+ (:count state) 1)))}
      :children [{:mode 'emitter-mode :id "emitter"}]
    })
    (pixils/defmode root-mode {
      :init (fn [state ctx] {:mid {:count 0}})
      :children [{:mode 'mid-mode :id "mid"
                  :state (pixils.ui/bind-state :mid)}]
    })
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);

  // When
  update_cycle();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->state->to_string(), "{:mid {:count 1}}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto& mid_mode = *session.active_mode->children[0];
  EXPECT_EQ(mid_mode.state->to_string(), "{:count 1}");
}

TEST_F(EventRoutingTest, child_override_on_map_merges_with_existing_event_handlers)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode emitter-mode {
      :update (fn [state ctx]
                (do (pixils.ui/emit! (:view ctx) :ping nil)
                    (pixils.ui/emit! (:view ctx) :pong nil)
                    state))
    })
    (pixils/defmode mid-mode {
      :on {:ping (fn [state event ctx]
                   (assoc state :ping-count (+ (:ping-count state) 1)))}
      :children [{:mode 'emitter-mode :id "emitter"}]
    })
    (pixils/defmode root-mode {
      :init (fn [state ctx] {:mid {:ping-count 0
                                   :pong-count 0}})
      :children [{:mode 'mid-mode
                  :id "mid"
                  :state (pixils.ui/bind-state :mid)
                  :on {:pong (fn [state event ctx]
                               (assoc state :pong-count (+ (:pong-count state) 1)))}}]
    })
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);

  // When
  update_cycle();

  // Then
  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->state->to_string(), "{:mid {:ping-count 1 :pong-count 1}}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto& mid_mode = *session.active_mode->children[0];
  EXPECT_EQ(mid_mode.state->to_string(), "{:ping-count 1 :pong-count 1}");
}

TEST_F(EventRoutingTest, custom_event_stop_propagation_prevents_ancestor_on_handler)
{
  runtime.eval(R"(
    (pixils/defmode emitter-mode {
      :update (fn [state ctx]
                (do (pixils.ui/emit! (:view ctx) :ping nil)
                    state))
    })
    (pixils/defmode mid-mode {
      :init (fn [state ctx] {:count 0})
      :on {:ping (fn [state event ctx]
                   (pixils.ui/stop-propagation! event)
                   (assoc state :count (+ (:count state) 1)))}
      :children [{:mode 'emitter-mode :id "emitter"}]
    })
    (pixils/defmode root-mode {
      :init (fn [state ctx] {:mid {:count 0}
                             :root-count 0})
      :on {:ping (fn [state event ctx]
                   (assoc state :root-count (+ (:root-count state) 1)))}
      :children [{:mode 'mid-mode
                  :id "mid"
                  :state (pixils.ui/bind-state :mid)}]
    })
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);

  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  EXPECT_EQ(session.active_mode->state->to_string(), "{:mid {:count 1} :root-count 0}");
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto& mid_mode = *session.active_mode->children[0];
  EXPECT_EQ(mid_mode.state->to_string(), "{:count 1}");
}

TEST_F(EventRoutingTest, non_rendered_child_does_not_win_hit_test_when_bounds_overlap)
{
  // Given - two overlapping children at the same position - top layer is hidden
  runtime.eval(R"(
    (pixils/defmode layer {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
    (pixils/defmode overlap-view {:children [{:mode 'layer :id "bg"}
                                             {:mode 'layer :id "fg" :style {:hidden true}}]})
  )");
  session.push_mode("overlap-view", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  // Both children cover the same area
  session.active_mode->children[0]->bounds = {0, 0, 200, 200};
  session.active_mode->children[1]->bounds = {0, 0, 200, 200};

  // When - click anywhere in the overlapping area
  input().mouse_down({100, 100});
  update_cycle();

  input().mouse_up({100, 100});
  update_cycle();

  // Then - the bg layer has received and processed the click event
  auto& bg_layer = *session.active_mode->children[0];
  EXPECT_EQ(bg_layer.state->to_string(), "{:clicks 1}");

  auto& fg_layer = *session.active_mode->children[1];
  EXPECT_EQ(fg_layer.state->to_string(), "{:clicks 0}");
}

TEST_F(EventRoutingTest, later_rendered_child_wins_hit_test_when_bounds_overlap)
{
  // Given - two overlapping children at the same position
  runtime.eval(R"(
    (pixils/defmode layer {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
    (pixils/defmode overlap-view {:children [{:mode 'layer :id "bg"}
                                             {:mode 'layer :id "fg"}]})
  )");
  session.push_mode("overlap-view", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  // Both children cover the same area
  session.active_mode->children[0]->bounds = {0, 0, 200, 200};
  session.active_mode->children[1]->bounds = {0, 0, 200, 200};

  // When - click anywhere in the overlapping area
  input().mouse_down({100, 100});
  update_cycle();

  input().mouse_up({100, 100});
  update_cycle();

  // Then - the second (fg) child, rendered on top, received the click
  auto& bg_layer = *session.active_mode->children[0];
  EXPECT_EQ(bg_layer.state->to_string(), "{:clicks 0}");

  auto& fg_layer = *session.active_mode->children[1];
  EXPECT_EQ(fg_layer.state->to_string(), "{:clicks 1}");
}

TEST_F(EventRoutingTest, interaction_hovered_true_when_cursor_is_inside)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode panel {
      :update (fn [state ctx] state)
    })
  )");
  session.push_mode("panel", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  // When - move cursor inside the view
  input().mouse_move({50, 50});
  update_cycle();

  // Then
  EXPECT_TRUE(session.active_mode->interaction.hovered);
}

TEST_F(EventRoutingTest, interaction_hovered_false_when_cursor_is_outside)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode panel {
      :update (fn [state ctx] state)
    })
  )");
  session.push_mode("panel", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  // When - move cursor outside the view
  input().mouse_move({150, 150});
  update_cycle();

  // Then
  EXPECT_FALSE(session.active_mode->interaction.hovered);
}

TEST_F(EventRoutingTest, mouse_down_sets_focus_to_deepest_hit_view_and_marks_ancestor_chain)
{
  runtime.eval(R"(
    (pixils/defmode child-mode {:focusable true})
    (pixils/defmode root-mode {:children [{:mode 'child-mode :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  update_cycle();

  auto focused = session.focus_state.focused.lock();
  ASSERT_NE(focused, nullptr);
  EXPECT_EQ(focused.get(), session.active_mode->children[0].get());

  ASSERT_EQ(session.focus_state.focus_chain.size(), 2u);
  EXPECT_EQ(session.focus_state.focus_chain[0].lock().get(),
            session.active_mode->children[0].get());
  EXPECT_EQ(session.focus_state.focus_chain[1].lock().get(), session.active_mode.get());

  EXPECT_TRUE(session.active_mode->children[0]->interaction.focused);
  EXPECT_TRUE(session.active_mode->children[0]->interaction.focus_within);
  EXPECT_FALSE(session.active_mode->interaction.focused);
  EXPECT_TRUE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, mouse_down_uses_nearest_focusable_ancestor_in_hit_chain)
{
  runtime.eval(R"(
    (pixils/defmode leaf-mode {})
    (pixils/defmode child-mode
      {:focusable true
       :children [{:mode 'leaf-mode :id "leaf"}]})
    (pixils/defmode root-mode {:children [{:mode 'child-mode :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};
  session.active_mode->children[0]->children[0]->bounds = {30, 30, 40, 40};

  input().mouse_down({40, 40});
  update_cycle();

  auto focused = session.focus_state.focused.lock();
  ASSERT_NE(focused, nullptr);
  EXPECT_EQ(focused.get(), session.active_mode->children[0].get());
}

TEST_F(EventRoutingTest, mouse_down_on_non_focusable_hit_chain_without_ancestor_clears_focus)
{
  runtime.eval(R"(
    (pixils/defmode focusable-child {:focusable true})
    (pixils/defmode static-child {})
    (pixils/defmode root-mode
      {:children [{:mode 'focusable-child :id "focusable"}
                  {:mode 'static-child :id "static"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 60, 60};
  session.active_mode->children[1]->bounds = {100, 20, 60, 60};

  input().mouse_down({40, 40});
  update_cycle();

  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.focus_state.focused.lock().get(),
            session.active_mode->children[0].get());

  input().mouse_down({120, 40});
  update_cycle();

  EXPECT_FALSE(session.focus_state.has_focus());
}

TEST_F(EventRoutingTest, mouse_down_outside_root_clears_focus)
{
  runtime.eval(R"(
    (pixils/defmode child-mode {:focusable true})
    (pixils/defmode root-mode {:children [{:mode 'child-mode :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  update_cycle();
  ASSERT_TRUE(session.focus_state.has_focus());

  input().mouse_down({250, 250});
  update_cycle();

  EXPECT_FALSE(session.focus_state.has_focus());
  EXPECT_FALSE(session.active_mode->interaction.focused);
  EXPECT_FALSE(session.active_mode->interaction.focus_within);
  EXPECT_FALSE(session.active_mode->children[0]->interaction.focused);
  EXPECT_FALSE(session.active_mode->children[0]->interaction.focus_within);
}

TEST_F(EventRoutingTest, focused_view_replacement_clears_focus_state)
{
  runtime.eval(R"(
    (pixils/defmode old-child {:focusable true})
    (pixils/defmode new-child {})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:swapped? false})
       :update (fn [state ctx]
                 (if (:swapped? state)
                   state
                   (do (pixils.ui/replace-child! (:view ctx)
                                                 "child"
                                                 {:mode 'new-child})
                       (assoc state :swapped? true))))
       :children [{:mode 'old-child :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  auto original_child = session.active_mode->children[0];

  input().mouse_down({50, 50});
  update_cycle();

  EXPECT_EQ(session.active_mode->children[0]->mode->name, "new-child");
  EXPECT_NE(session.active_mode->children[0].get(), original_child.get());
  EXPECT_FALSE(session.focus_state.has_focus());
  EXPECT_FALSE(session.active_mode->interaction.focused);
  EXPECT_FALSE(session.active_mode->interaction.focus_within);
  EXPECT_FALSE(session.active_mode->children[0]->interaction.focused);
  EXPECT_FALSE(session.active_mode->children[0]->interaction.focus_within);
}

TEST_F(EventRoutingTest, focused_view_replacement_falls_back_to_focusable_ancestor)
{
  runtime.eval(R"(
    (pixils/defmode old-child {:focusable true})
    (pixils/defmode new-child {})
    (pixils/defmode root-mode
      {:focusable true
       :init (fn [state ctx] {:swapped? false})
       :update (fn [state ctx]
                 (if (:swapped? state)
                   state
                   (do (pixils.ui/replace-child! (:view ctx)
                                                 "child"
                                                 {:mode 'new-child})
                       (assoc state :swapped? true))))
       :children [{:mode 'old-child :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  update_cycle();

  EXPECT_EQ(session.active_mode->children[0]->mode->name, "new-child");
  ASSERT_TRUE(session.focus_state.has_focus());
  EXPECT_EQ(session.focus_state.focused.lock().get(), session.active_mode.get());
  EXPECT_TRUE(session.active_mode->interaction.focused);
  EXPECT_TRUE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, hidden_focused_view_falls_back_to_focusable_ancestor)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:focusable true
       :on-click (fn [state event ctx]
                   (do
                     (pixils.ui/style! ctx {:hidden true})
                     state))})
    (pixils/defmode root-mode
      {:focusable true
       :children [{:mode 'child-mode :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  update_cycle();
  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.focus_state.focused.lock().get(),
            session.active_mode->children[0].get());

  input().mouse_up({50, 50});
  update_cycle();

  ASSERT_TRUE(session.focus_state.has_focus());
  EXPECT_EQ(session.focus_state.focused.lock().get(), session.active_mode.get());
  EXPECT_TRUE(session.active_mode->interaction.focused);
  EXPECT_TRUE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, pop_mode_restores_focus_from_underlying_frame)
{
  runtime.eval(R"(
    (pixils/defmode child-mode {:focusable true})
    (pixils/defmode root-mode
      {:focusable true
       :children [{:mode 'child-mode :id "child"}]})
    (pixils/defmode popup-mode {:focusable true})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  update_cycle();
  auto focused_child = session.active_mode->children[0];
  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.focus_state.focused.lock().get(), focused_child.get());

  session.push_mode("popup-mode", Roo::Constant::NIL);
  EXPECT_FALSE(session.focus_state.has_focus());

  session.pop_mode();

  ASSERT_TRUE(session.focus_state.has_focus());
  EXPECT_EQ(session.focus_state.focused.lock().get(), focused_child.get());
  EXPECT_TRUE(session.active_mode->children[0]->interaction.focused);
  EXPECT_TRUE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, focus_style_variants_apply_to_focused_leaf_and_ancestor_chain)
{
  runtime.eval(R"(
    (pixils/defmode child-mode
      {:focusable true
       :style (pixils.ui.style/make-style
                {:width 40
                 :focus {:width 90}})})
    (pixils/defmode root-mode
      {:style (pixils.ui.style/make-style
                {:height 10
                 :focus-within {:height 30}})
       :children [{:mode 'child-mode :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  session.active_mode->children[0]->bounds = {20, 20, 100, 100};

  input().mouse_down({50, 50});
  update_cycle();
  session.render_mode();

  auto root_style = Pixils::UI::resolve_style(session.active_mode->mode->style,
                                              session.active_mode->state,
                                              session.active_mode->interaction);
  auto child_style =
    Pixils::UI::resolve_style(session.active_mode->children[0]->mode->style,
                              session.active_mode->children[0]->state,
                              session.active_mode->children[0]->interaction);

  ASSERT_NE(root_style.height, std::nullopt);
  EXPECT_TRUE(root_style.height->is_fixed());
  EXPECT_EQ(root_style.height->fixed_value_or(0), 30);

  ASSERT_NE(child_style.width, std::nullopt);
  EXPECT_TRUE(child_style.width->is_fixed());
  EXPECT_EQ(child_style.width->fixed_value_or(0), 90);
}

TEST_F(EventRoutingTest, hover_style_variant_applied_when_cursor_is_inside)
{
  // Given - a mode with a hover style that changes width
  runtime.eval(R"(
    (pixils/defmode panel {
      :style (pixils.ui.style/make-style
               {:width 100
                :hover (pixils.ui.style/make-style {:width 200})})
    })
  )");
  session.push_mode("panel", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  // When - move cursor inside the view
  input().mouse_move({50, 50});
  update_cycle();

  // Then - resolved style should reflect the hover variant
  auto style = Pixils::UI::resolve_style(session.active_mode->mode->style,
                                         session.active_mode->state,
                                         session.active_mode->interaction);
  ASSERT_NE(style.width, std::nullopt);
  EXPECT_TRUE(style.width->is_fixed());
  EXPECT_EQ(style.width->fixed_value_or(0), 200);
}

TEST_F(EventRoutingTest,
       scaled_child_hit_testing_uses_external_footprint_and_logical_local_position)
{
  runtime.eval(R"(
    (pixils/defmode scaled-child
      {:style {:width 100 :height 50 :scale 2}
       :on-mouse-down (fn [state event ctx]
                        {:x (:x (:position event))
                         :y (:y (:position event))})})

    (pixils/defmode root-mode
      {:children [{:mode 'scaled-child :id "child"}]})
  )");
  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 300, 200};
  session.active_mode->children[0]->bounds = {0, 0, 100, 50};

  input().mouse_down({150, 20});
  update_cycle();

  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->state->to_string(), "{:x 75 :y 10}");
}

TEST_F(EventRoutingTest, focus_and_blur_bang_update_focus_state_from_hook_context)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:focusable true
       :init (fn [state ctx] {:step 0})
       :update (fn [state ctx]
                   (if (= (:step state) 0)
                   (do (pixils.ui/focus! ctx)
                       (assoc state :step 1))
                   (if (= (:step state) 1)
                     (do (pixils.ui/blur!)
                         (assoc state :step 2))
                     state)))
       :style (pixils.ui.style/make-style
                {:width 40
                 :focus {:width 90}})})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  update_cycle();

  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.focus_state.focused.lock().get(), session.active_mode.get());
  EXPECT_TRUE(session.active_mode->interaction.focused);
  EXPECT_TRUE(session.active_mode->interaction.focus_within);

  auto focused_style = Pixils::UI::resolve_style(session.active_mode->mode->style,
                                                 session.active_mode->state,
                                                 session.active_mode->interaction);
  ASSERT_NE(focused_style.width, std::nullopt);
  EXPECT_EQ(focused_style.width->fixed_value_or(0), 90);

  update_cycle();

  EXPECT_FALSE(session.focus_state.has_focus());
  EXPECT_FALSE(session.active_mode->interaction.focused);
  EXPECT_FALSE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, focus_bang_on_non_focusable_view_does_not_change_focus_state)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx]
               (do (pixils.ui/focus! ctx)
                   state))})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  update_cycle();

  EXPECT_FALSE(session.focus_state.has_focus());
  EXPECT_FALSE(session.active_mode->interaction.focused);
  EXPECT_FALSE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, pushed_mode_init_focuses_itself_in_same_process_messages_cycle)
{
  runtime.eval(R"(
    (pixils/defmode popup-mode
      {:focusable true
       :init (fn [state ctx]
               (pixils.ui/focus! ctx)
               state)})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:opened? false})
       :update (fn [state ctx]
                 (if (:opened? state)
                   state
                   (do (pixils/push-mode! 'popup-mode)
                       (assoc state :opened? true))))})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  update_cycle();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_NE(session.active_mode->mode, nullptr);
  EXPECT_EQ(session.active_mode->mode->name, "popup-mode");
  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.focus_state.focused.lock().get(), session.active_mode.get());
  EXPECT_TRUE(session.active_mode->interaction.focused);
  EXPECT_TRUE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, init_focused_view_survives_following_update_before_first_layout)
{
  runtime.eval(R"(
    (pixils/defmode popup-mode
      {:focusable true
       :init (fn [state ctx]
               (pixils.ui/focus! ctx)
               state)})
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:opened? false})
       :update (fn [state ctx]
                 (if (:opened? state)
                   state
                   (do (pixils/push-mode! 'popup-mode)
                       (assoc state :opened? true))))})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  update_cycle();
  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.active_mode->mode->name, "popup-mode");

  update_cycle();

  ASSERT_TRUE(session.focus_state.has_focus());
  ASSERT_EQ(session.focus_state.focused.lock().get(), session.active_mode.get());
  EXPECT_TRUE(session.active_mode->interaction.focused);
  EXPECT_TRUE(session.active_mode->interaction.focus_within);
}

TEST_F(EventRoutingTest, quit_bang_requests_session_shutdown)
{
  runtime.eval("(pixils/quit!)");

  EXPECT_FALSE(session.quit_requested);

  session.process_messages();

  EXPECT_TRUE(session.quit_requested);
}
