
#include "session_fixture.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

class EventRoutingTest : public SessionFixture
{
 protected:
  void move_mouse(int x, int y) { events.do_mouse_motion(x, y); }

  void press_at(int x, int y)
  {
    move_mouse(x, y);
    SDL_MouseButtonEvent btn{};
    btn.button = SDL_BUTTON_LEFT;
    events.do_mouse_button_down(btn);
  }

  void release_at(int x, int y)
  {
    move_mouse(x, y);
    SDL_MouseButtonEvent btn{};
    btn.button = SDL_BUTTON_LEFT;
    events.do_mouse_button_up(btn);
  }

  /**
   * Clear per-frame transient events, leaving mouse_held untouched so the
   * pressed chain stays intact across frames while the button is held.
   */
  void next_frame()
  {
    events.mouse_button_down = Lisple::Constant::NIL;
    events.mouse_button_up = Lisple::Constant::NIL;
  }

  Lisple::sptr_rtval get_state_key(const Lisple::sptr_rtval& state, const std::string& key)
  {
    return Lisple::Dict::get_property(state, Lisple::RTValue::keyword(key));
  }

  int get_count(const Lisple::sptr_rtval& state, const std::string& key)
  {
    auto val = get_state_key(state, key);
    if (!val || val->type == Lisple::RTValue::Type::NIL) return 0;
    return val->num().get_int();
  }
};

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
  press_at(50, 50);
  session.update_mode();

  // When - mouse-up at same position
  next_frame();
  release_at(50, 50);
  session.update_mode();

  // Then
  EXPECT_EQ(get_count(session.active_mode->state, "clicks"), 1);
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
  press_at(25, 50);
  session.update_mode();

  /**
   * Move the cursor to the right view in a separate frame so that
   * mouse.hovered is updated before handle_mouse_up runs. This matches
   * real usage: SDL delivers motion and button events in separate frames.
   */
  next_frame();
  move_mouse(150, 50);
  session.update_mode();

  // When - release on right button
  next_frame();
  release_at(150, 50);
  session.update_mode();

  // Then - neither button received a click
  auto left_state = get_state_key(session.active_mode->state, "left");
  auto right_state = get_state_key(session.active_mode->state, "right");
  EXPECT_EQ(get_count(left_state, "clicks"), 0);
  EXPECT_EQ(get_count(right_state, "clicks"), 0);
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
  press_at(25, 50);
  session.update_mode();

  next_frame();
  move_mouse(150, 50);
  session.update_mode();

  next_frame();
  release_at(150, 50);
  session.update_mode();

  // Then - right button received on-mouse-up even though press started on left
  auto right_state = get_state_key(session.active_mode->state, "right");
  EXPECT_EQ(get_count(right_state, "up-count"), 1);
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
  press_at(50, 50);
  session.update_mode();

  next_frame();
  release_at(50, 50);
  session.update_mode();

  // Then - updated child state is visible in the parent's state map
  auto btn_state = get_state_key(session.active_mode->state, "btn");
  ASSERT_NE(btn_state, nullptr);
  EXPECT_EQ(get_count(btn_state, "clicks"), 1);
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
  press_at(50, 50);
  session.update_mode();

  // Then
  auto btn_state = get_state_key(session.active_mode->state, "btn");
  ASSERT_NE(btn_state, nullptr);
  EXPECT_EQ(get_count(btn_state, "pressed-count"), 1);
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
  move_mouse(5, 5);
  session.update_mode();

  next_frame();
  move_mouse(50, 50);
  session.update_mode();

  // Then
  auto panel_state = get_state_key(session.active_mode->state, "panel");
  ASSERT_NE(panel_state, nullptr);
  EXPECT_EQ(get_count(panel_state, "entered"), 1);
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
  move_mouse(50, 50);
  session.update_mode();

  next_frame();
  move_mouse(5, 5);
  session.update_mode();

  // Then
  auto panel_state = get_state_key(session.active_mode->state, "panel");
  ASSERT_NE(panel_state, nullptr);
  EXPECT_EQ(get_count(panel_state, "left-count"), 1);
}

TEST_F(EventRoutingTest, later_rendered_child_wins_hit_test_when_bounds_overlap)
{
  // Given - two overlapping children at the same position; only the second tracks clicks
  runtime.eval(R"(
    (pixils/defmode bg-layer {})
    (pixils/defmode fg-layer {
      :init     (fn [state ctx] {:clicks 0})
      :on-click (fn [state ev ctx] (assoc state :clicks (+ (:clicks state) 1)))
    })
    (pixils/defmode overlap-view {:children [{:mode 'bg-layer :id "bg"}
                                             {:mode 'fg-layer :id "fg"}]})
  )");
  session.push_mode("overlap-view", Lisple::Constant::NIL);
  session.active_mode->bounds = {0, 0, 200, 200};
  // Both children cover the same area
  session.active_mode->children[0]->bounds = {0, 0, 200, 200};
  session.active_mode->children[1]->bounds = {0, 0, 200, 200};

  // When - click anywhere in the overlapping area
  press_at(100, 100);
  session.update_mode();

  next_frame();
  release_at(100, 100);
  session.update_mode();

  // Then - the second (fg) child, rendered on top, received the click
  auto fg_state = get_state_key(session.active_mode->state, "fg");
  ASSERT_NE(fg_state, nullptr);
  EXPECT_EQ(get_count(fg_state, "clicks"), 1);
}

TEST_F(EventRoutingTest, inject_booleans_sets_hovered_true_when_cursor_is_inside)
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
  move_mouse(50, 50);
  session.update_mode();

  // Then
  auto hovered = get_state_key(session.active_mode->state, "hovered");
  ASSERT_NE(hovered, nullptr);
  EXPECT_EQ(*hovered, *Lisple::Constant::BOOL_TRUE);
}

TEST_F(EventRoutingTest, inject_booleans_sets_hovered_false_when_cursor_is_outside)
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
  move_mouse(150, 150);
  session.update_mode();

  // Then
  auto hovered = get_state_key(session.active_mode->state, "hovered");
  ASSERT_NE(hovered, nullptr);
  EXPECT_EQ(*hovered, *Lisple::Constant::BOOL_FALSE);
}
