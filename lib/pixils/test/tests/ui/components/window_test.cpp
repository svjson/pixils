#include "../../render_fixture.h"

#include <SDL2/SDL_mouse.h>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>

using WindowTest = RenderFixture;

namespace
{
  Roo::sptr_val get_key(const Roo::sptr_val& value, const std::string& key)
  {
    return Roo::Dict::get_property(value, Roo::keyword(key));
  }

} // namespace

#define SETTLE_LAYOUT() \
  do                    \
  {                     \
    update_cycle();     \
    render_cycle();     \
    update_cycle();     \
    render_cycle();     \
  } while (false)

TEST_F(WindowTest, dragged_window_keeps_latest_position_after_follow_up_interaction)
{
  runtime.eval(R"(
    (pixils/defcomponent probe-body
      {:style {:width 80
               :height 40}
       :on-mouse-down (fn [state event ctx]
                        (assoc state :presses (+ (or (:presses state) 0) 1)))})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:position {:x 24 :y 20}
                    :style {:width 80
                            :height 56}
                    :title-bar {:title "Probe"}
                    :body [{:mode 'probe-body
                            :state {:presses 0}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  SETTLE_LAYOUT();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto window = session.active_mode->children[0];
  ASSERT_NE(window, nullptr);
  ASSERT_EQ(window->children.size(), 2u);
  auto title_bar = window->children[0];
  auto body = window->children[1];
  ASSERT_NE(title_bar, nullptr);
  ASSERT_NE(body, nullptr);

  EXPECT_EQ(window->bounds.x, 24);
  EXPECT_EQ(window->bounds.y, 20);

  const int drag_dx = 31;
  const int drag_dy = 17;
  const int start_x = title_bar->bounds.x + (title_bar->bounds.w / 2);
  const int start_y = title_bar->bounds.y + (title_bar->bounds.h / 2);
  input().mouse_down({start_x, start_y}, SDL_BUTTON_LEFT);
  update_cycle();
  input().mouse_move({start_x + drag_dx, start_y + drag_dy});
  update_cycle();
  render_cycle();

  window = session.active_mode->children[0];
  body = window->children[1];
  ASSERT_NE(window, nullptr);
  ASSERT_NE(body, nullptr);
  auto position = get_key(window->state, "position");
  ASSERT_NE(position, nullptr);
  EXPECT_EQ(get_key(position, "x")->num().get_int(), 24 + drag_dx);
  EXPECT_EQ(get_key(position, "y")->num().get_int(), 20 + drag_dy);

  input().mouse_up({start_x + drag_dx, start_y + drag_dy}, SDL_BUTTON_LEFT);
  update_cycle();
  render_cycle();

  window = session.active_mode->children[0];
  body = window->children[1];
  ASSERT_NE(window, nullptr);
  ASSERT_NE(body, nullptr);
  EXPECT_EQ(window->bounds.x, 24 + drag_dx);
  EXPECT_EQ(window->bounds.y, 20 + drag_dy);

  ASSERT_EQ(body->children.size(), 1u);
  auto probe_body = body->children[0];
  ASSERT_NE(probe_body, nullptr);

  input().mouse_down({probe_body->bounds.x + 4, probe_body->bounds.y + 4}, SDL_BUTTON_LEFT);
  update_cycle();
  render_cycle();

  window = session.active_mode->children[0];
  body = window->children[1];
  ASSERT_NE(window, nullptr);
  ASSERT_NE(body, nullptr);
  EXPECT_EQ(window->bounds.x, 24 + drag_dx);
  EXPECT_EQ(window->bounds.y, 20 + drag_dy);

  ASSERT_EQ(body->children.size(), 1u);
  probe_body = body->children[0];
  ASSERT_NE(probe_body, nullptr);
  auto presses = get_key(probe_body->state, "presses");
  ASSERT_NE(presses, nullptr);
  EXPECT_EQ(presses->num().get_int(), 1);
}
