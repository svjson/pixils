
#include "session_fixture.h"
#include <pixils/ui/style.h>

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

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
  session.push_mode("clickable", Lisple::Constant::NIL);
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
  session.push_mode("split-view", Lisple::Constant::NIL);
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
  session.push_mode("split-view", Lisple::Constant::NIL);
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
  session.push_mode("container-mode", Lisple::Constant::NIL);
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
  session.push_mode("container-mode", Lisple::Constant::NIL);
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
  session.push_mode("container-mode", Lisple::Constant::NIL);
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
  session.push_mode("container-mode", Lisple::Constant::NIL);
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

TEST_F(EventRoutingTest, later_rendered_child_wins_hit_test_when_bounds_overlap)
{
  // Given - two overlapping children at the same position; only the second tracks clicks
  runtime.eval(R"(
    (pixils/defmode layer {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
    (pixils/defmode overlap-view {:children [{:mode 'layer :id "bg"}
                                             {:mode 'layer :id "fg"}]})
  )");
  session.push_mode("overlap-view", Lisple::Constant::NIL);
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
  session.push_mode("panel", Lisple::Constant::NIL);
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
  session.push_mode("panel", Lisple::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  // When - move cursor outside the view
  input().mouse_move({150, 150});
  update_cycle();

  // Then
  EXPECT_FALSE(session.active_mode->interaction.hovered);
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
  session.push_mode("panel", Lisple::Constant::NIL);
  session.active_mode->bounds = {0, 0, 100, 100};

  // When - move cursor inside the view
  input().mouse_move({50, 50});
  update_cycle();

  // Then - resolved style should reflect the hover variant
  auto style = Pixils::UI::resolve_style(session.active_mode->mode->style,
                                         session.active_mode->state,
                                         session.active_mode->interaction);
  EXPECT_EQ(style.width.value_or(0), 200);
}
