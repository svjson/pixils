#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <roo/runtime/dict.h>

using WindowTest = RenderFixture;
using Pixils::Runtime::View;

namespace
{
  std::shared_ptr<View> find_first_mode(const std::shared_ptr<View>& view,
                                        const std::string& mode_name)
  {
    if (!view) return nullptr;
    if (view->mode && view->mode->name == mode_name) return view;

    for (const auto& child : view->children)
    {
      auto match = find_first_mode(child, mode_name);
      if (match) return match;
    }

    return nullptr;
  }

  void set_state_number(const std::shared_ptr<View>& view, const std::string& key, int value)
  {
    auto next = Roo::Dict::shallow_copy(view->state);
    Roo::Dict::set_property(next, Roo::keyword(key), Roo::number(value));
    view->state = next;
  }
} // namespace

TEST_F(WindowTest, auto_positioned_window_keeps_existing_recenter_behavior)
{
  runtime.eval(R"(
    (pixils/defcomponent test/resizable-window-body
      {:update (fn [state ctx]
                 (pixils.ui/style! (:view ctx)
                                   {:width (:w state)
                                    :height (:h state)})
                 state)})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Dynamic"}
                    :body [{:mode 'test/resizable-window-body
                            :state {:w 80 :h 40}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto window = find_first_mode(session.active_mode, "ui/window");
  auto body = find_first_mode(session.active_mode, "test/resizable-window-body");
  ASSERT_NE(window, nullptr);
  ASSERT_NE(body, nullptr);
  EXPECT_EQ(window->bounds.x, (320 - window->bounds.w) / 2);
  EXPECT_EQ(window->bounds.y, (200 - window->bounds.h) / 2);

  set_state_number(body, "w", 160);
  frame_cycle();

  window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, (320 - window->bounds.w) / 2);
  EXPECT_EQ(window->bounds.y, (200 - window->bounds.h) / 2);
}

TEST_F(WindowTest, explicit_position_remains_unconstrained_by_default)
{
  runtime.eval(R"(
    (pixils/defcomponent test/fixed-window-body
      {:update (fn [state ctx]
                 (pixils.ui/style! (:view ctx)
                                   {:width 80
                                    :height 40})
                 state)})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Explicit"}
                    :position {:x 280 :y 180}
                    :body [{:mode 'test/fixed-window-body}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, 280);
  EXPECT_EQ(window->bounds.y, 180);
}

TEST_F(WindowTest, resize_keep_placement_preserves_initial_center)
{
  runtime.eval(R"(
    (pixils/defcomponent test/resizable-window-body
      {:update (fn [state ctx]
                 (pixils.ui/style! (:view ctx)
                                   {:width (:w state)
                                    :height (:h state)})
                 state)})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Pinned"}
                    :placement {:initial {:x :center :y :center}
                                :resize {:x :keep :y :keep}}
                    :body [{:mode 'test/resizable-window-body
                            :state {:w 80 :h 40}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto window = find_first_mode(session.active_mode, "ui/window");
  auto body = find_first_mode(session.active_mode, "test/resizable-window-body");
  ASSERT_NE(window, nullptr);
  ASSERT_NE(body, nullptr);
  const int initial_x = window->bounds.x;
  const int initial_y = window->bounds.y;
  EXPECT_EQ(initial_x, (320 - window->bounds.w) / 2);
  EXPECT_EQ(initial_y, (200 - window->bounds.h) / 2);

  set_state_number(body, "w", 160);
  frame_cycle();

  window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, initial_x);
  EXPECT_EQ(window->bounds.y, initial_y);
}

TEST_F(WindowTest, constraints_can_contain_explicit_position)
{
  runtime.eval(R"(
    (pixils/defcomponent test/fixed-window-body
      {:update (fn [state ctx]
                 (pixils.ui/style! (:view ctx)
                                   {:width 80
                                    :height 40})
                 state)})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Contained"}
                    :position {:x 280 :y 180}
                    :constraints {:bounds :parent
                                  :x :contain
                                  :y :contain}
                    :body [{:mode 'test/fixed-window-body}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, 320 - window->bounds.w);
  EXPECT_EQ(window->bounds.y, 200 - window->bounds.h);
}

TEST_F(WindowTest, constraints_apply_to_auto_positioned_window_without_placement)
{
  runtime.eval(R"(
    (pixils/defcomponent test/fixed-window-body
      {:update (fn [state ctx]
                 (pixils.ui/style! (:view ctx)
                                   {:width 80
                                    :height 40})
                 state)})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Region"}
                    :constraints {:bounds {:x 0 :y 0 :w 120 :h 80}
                                  :x :contain
                                  :y :contain}
                    :body [{:mode 'test/fixed-window-body}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, 120 - window->bounds.w);
  EXPECT_EQ(window->bounds.y, 80 - window->bounds.h);
}

TEST_F(WindowTest, explicit_position_is_initial_when_resize_placement_recenters)
{
  runtime.eval(R"(
    (pixils/defcomponent test/resizable-window-body
      {:update (fn [state ctx]
                 (pixils.ui/style! (:view ctx)
                                   {:width (:w state)
                                    :height (:h state)})
                 state)})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Explicit"}
                    :position {:x 30 :y 20}
                    :placement {:resize {:x :center
                                         :y :keep}}
                    :body [{:mode 'test/resizable-window-body
                            :state {:w 80 :h 40}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto window = find_first_mode(session.active_mode, "ui/window");
  auto body = find_first_mode(session.active_mode, "test/resizable-window-body");
  ASSERT_NE(window, nullptr);
  ASSERT_NE(body, nullptr);
  EXPECT_EQ(window->bounds.x, 30);
  EXPECT_EQ(window->bounds.y, 20);

  frame_cycle();

  window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, 30);
  EXPECT_EQ(window->bounds.y, 20);

  set_state_number(body, "w", 160);
  frame_cycle();

  window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, (320 - window->bounds.w) / 2);
  EXPECT_EQ(window->bounds.y, 20);
}

TEST_F(WindowTest, center_on_overflow_recenters_only_overflowing_axis)
{
  runtime.eval(R"(
    (pixils/defcomponent test/resizable-window-body
      {:update (fn [state ctx]
                 (pixils.ui/style! (:view ctx)
                                   {:width (:w state)
                                    :height (:h state)})
                 state)})

    (pixils/defmode root-mode
      {:children [(pixils.ui.window/make
                   {:title-bar {:title "Overflow"}
                    :position {:x 30 :y 20}
                    :placement {:resize {:x :keep
                                         :y :keep}}
                    :constraints {:bounds :parent
                                  :x :center-on-overflow
                                  :y :contain}
                    :body [{:mode 'test/resizable-window-body
                            :state {:w 80 :h 40}}]})]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  frame_cycle();

  auto window = find_first_mode(session.active_mode, "ui/window");
  auto body = find_first_mode(session.active_mode, "test/resizable-window-body");
  ASSERT_NE(window, nullptr);
  ASSERT_NE(body, nullptr);
  EXPECT_EQ(window->bounds.x, 30);
  EXPECT_EQ(window->bounds.y, 20);

  set_state_number(body, "w", 300);
  frame_cycle();

  window = find_first_mode(session.active_mode, "ui/window");
  ASSERT_NE(window, nullptr);
  EXPECT_EQ(window->bounds.x, (320 - window->bounds.w) / 2);
  EXPECT_EQ(window->bounds.y, 20);
}
