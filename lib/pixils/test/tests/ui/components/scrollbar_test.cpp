#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

#include <algorithm>

using ScrollbarTest = RenderFixture;

namespace
{
  Lisple::sptr_val get_state_key(const std::shared_ptr<Pixils::Runtime::View>& view,
                                 const std::string& key)
  {
    return Lisple::Dict::get_property(view->state, Lisple::keyword(key));
  }
} // namespace

TEST_F(ScrollbarTest, scrollbar_lays_out_button_children_from_axis)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:width 50 :height 10}
                   :state {:axis :x :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);

  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto start_button = scrollbar->children[0];
  auto spacer = scrollbar->children[1];
  auto end_button = scrollbar->children[2];
  ASSERT_NE(start_button, nullptr);
  ASSERT_NE(spacer, nullptr);
  ASSERT_NE(end_button, nullptr);

  EXPECT_EQ(start_button->bounds.x, 0);
  EXPECT_EQ(start_button->bounds.y, 0);
  EXPECT_EQ(start_button->bounds.w, 10);
  EXPECT_EQ(start_button->bounds.h, 10);

  EXPECT_EQ(spacer->bounds.x, 10);
  EXPECT_EQ(spacer->bounds.y, 0);
  EXPECT_EQ(spacer->bounds.w, 30);
  EXPECT_EQ(spacer->bounds.h, 10);

  EXPECT_EQ(end_button->bounds.x, 40);
  EXPECT_EQ(end_button->bounds.y, 0);
  EXPECT_EQ(end_button->bounds.w, 10);
  EXPECT_EQ(end_button->bounds.h, 10);
}

TEST_F(ScrollbarTest, vertical_scrollbar_shrinks_parts_when_axis_is_too_short)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:width 14 :height 20}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto start_button = scrollbar->children[0];
  auto track = scrollbar->children[1];
  auto end_button = scrollbar->children[2];
  ASSERT_NE(start_button, nullptr);
  ASSERT_NE(track, nullptr);
  ASSERT_NE(end_button, nullptr);
  ASSERT_EQ(track->children.size(), 1u);
  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);

  EXPECT_LT(start_button->bounds.h, 14);
  EXPECT_EQ(start_button->bounds.y, scrollbar->bounds.y);
  EXPECT_EQ(track->bounds.y, start_button->bounds.y + start_button->bounds.h);
  EXPECT_EQ(end_button->bounds.y + end_button->bounds.h,
            scrollbar->bounds.y + scrollbar->bounds.h);
  EXPECT_LE(handle->bounds.y + handle->bounds.h, track->bounds.y + track->bounds.h);
  EXPECT_LE(start_button->bounds.h + track->bounds.h + end_button->bounds.h,
            scrollbar->bounds.h);
}

TEST_F(ScrollbarTest, horizontal_scrollbar_shrinks_parts_when_axis_is_too_short)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:width 20 :height 14}
                   :state {:axis :x :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto start_button = scrollbar->children[0];
  auto track = scrollbar->children[1];
  auto end_button = scrollbar->children[2];
  ASSERT_NE(start_button, nullptr);
  ASSERT_NE(track, nullptr);
  ASSERT_NE(end_button, nullptr);
  ASSERT_EQ(track->children.size(), 1u);
  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);

  EXPECT_LT(start_button->bounds.w, 14);
  EXPECT_EQ(start_button->bounds.x, scrollbar->bounds.x);
  EXPECT_EQ(track->bounds.x, start_button->bounds.x + start_button->bounds.w);
  EXPECT_EQ(end_button->bounds.x + end_button->bounds.w,
            scrollbar->bounds.x + scrollbar->bounds.w);
  EXPECT_LE(handle->bounds.x + handle->bounds.w, track->bounds.x + track->bounds.w);
  EXPECT_LE(start_button->bounds.w + track->bounds.w + end_button->bounds.w,
            scrollbar->bounds.w);
}

TEST_F(ScrollbarTest, scrollbar_button_children_bubble_click_behavior_to_scrollbar)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:width 50 :height 10}
                   :state {:axis :x :content-size 100 :value 0 :step 5}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);

  input().mouse_down({45, 5});
  update_cycle();

  auto value = Lisple::Dict::get_property(scrollbar->state, Lisple::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 5);
}

TEST_F(ScrollbarTest, proportional_handle_uses_viewport_size_for_page_ratio)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 400 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto track = scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);

  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);
  EXPECT_EQ(scrollbar->bounds.h, 200);
  EXPECT_EQ(track->bounds.h, 172);
  EXPECT_EQ(handle->bounds.h, 82);
}

TEST_F(ScrollbarTest, proportional_handle_at_max_reaches_track_end)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 400 :value 200}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto track = scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);

  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);
  EXPECT_EQ(handle->bounds.y + handle->bounds.h, track->bounds.y + track->bounds.h);
}

TEST_F(ScrollbarTest, dragging_proportional_handle_to_end_sets_max_value)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 400 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);

  input().mouse_down({scrollbar->bounds.x + 5, scrollbar->bounds.y + 20});
  update_cycle();
  input().mouse_move({scrollbar->bounds.x + 5,
                      scrollbar->bounds.y + scrollbar->bounds.h - 15});
  update_cycle();

  auto value = get_state_key(scrollbar, "value");
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 200);
}

TEST_F(ScrollbarTest, scrollbar_handle_pressed_state_clears_after_click)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 400 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);
  auto track = scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);
  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);

  input().mouse_down({5, 20});
  update_cycle();

  auto active_part = get_state_key(scrollbar, "active-part");
  auto pressed = get_state_key(handle, "pressed");
  ASSERT_NE(active_part, nullptr);
  ASSERT_NE(pressed, nullptr);
  EXPECT_EQ(active_part->to_string(), ":handle");
  EXPECT_EQ(pressed->to_string(), "true");

  input().mouse_up({5, 20});
  update_cycle();
  update_cycle();

  active_part = get_state_key(scrollbar, "active-part");
  pressed = get_state_key(handle, "pressed");
  ASSERT_NE(active_part, nullptr);
  ASSERT_NE(pressed, nullptr);
  EXPECT_EQ(active_part->to_string(), "nil");
  EXPECT_EQ(pressed->to_string(), "false");
}

TEST_F(ScrollbarTest, base_theme_scrollbar_buttons_use_generated_outline_arrows)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:children [{:mode 'ui/scrollbar
                   :style {:width 50 :height 14}
                   :state {:axis :x :content-size 100 :value 0}}
                  {:mode 'ui/scrollbar
                   :style {:width 14 :height 50}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  runtime.eval(R"(
    (defun resource-size [bundle id]
      (:size
       (head
        (filter (pixils.resource/list-images bundle)
                (fn [resource]
                  (= (:id resource) id))))))
  )");

  auto up = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :base-theme)
                           (fn [resource]
                             (= (:id resource) :base-theme/scrollbar-arrow-up)))))
  )");
  auto down = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :base-theme)
                           (fn [resource]
                             (= (:id resource) :base-theme/scrollbar-arrow-down)))))
  )");
  auto left = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :base-theme)
                           (fn [resource]
                             (= (:id resource) :base-theme/scrollbar-arrow-left)))))
  )");
  auto right = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :base-theme)
                           (fn [resource]
                             (= (:id resource) :base-theme/scrollbar-arrow-right)))))
  )");

  ASSERT_NE(up, nullptr);
  ASSERT_NE(down, nullptr);
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(up->to_string(), ":generated");
  EXPECT_EQ(down->to_string(), ":generated");
  EXPECT_EQ(left->to_string(), ":generated");
  EXPECT_EQ(right->to_string(), ":generated");

  EXPECT_EQ(runtime.eval("(:w (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            9);
  EXPECT_EQ(runtime.eval("(:h (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            9);
  EXPECT_EQ(runtime.eval("(:w (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            9);
  EXPECT_EQ(runtime.eval("(:h (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            9);
  EXPECT_EQ(runtime.eval("(:w (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            9);
  EXPECT_EQ(runtime.eval("(:h (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            9);
  EXPECT_EQ(runtime.eval("(:w (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            9);
  EXPECT_EQ(runtime.eval("(:h (resource-size :base-theme "
                         ":base-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            9);

  auto copy_ops = std::count_if(render_target()->render_ops.begin(),
                                render_target()->render_ops.end(),
                                [](const auto& op)
                                { return op.type == RenderOpType::RENDER_COPY; });
  EXPECT_GE(copy_ops, 4);
}

TEST_F(ScrollbarTest, windows_95_scrollbar_buttons_use_generated_theme_symbols)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :children [{:mode 'ui/scrollbar
                   :style {:width 50 :height 10}
                   :state {:axis :x :content-size 100 :value 0}}
                  {:mode 'ui/scrollbar
                   :style {:width 10 :height 50}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  runtime.eval(R"(
    (defun resource-size [bundle id]
      (:size
       (head
        (filter (pixils.resource/list-images bundle)
                (fn [resource]
                  (= (:id resource) id))))))
  )");

  auto up = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-95-theme)
                           (fn [resource]
                             (= (:id resource) :windows-95-theme/scrollbar-arrow-up)))))
  )");
  auto down = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-95-theme)
                           (fn [resource]
                             (= (:id resource) :windows-95-theme/scrollbar-arrow-down)))))
  )");
  auto left = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-95-theme)
                           (fn [resource]
                             (= (:id resource) :windows-95-theme/scrollbar-arrow-left)))))
  )");
  auto right = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-95-theme)
                           (fn [resource]
                             (= (:id resource) :windows-95-theme/scrollbar-arrow-right)))))
  )");

  ASSERT_NE(up, nullptr);
  ASSERT_NE(down, nullptr);
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(up->to_string(), ":generated");
  EXPECT_EQ(down->to_string(), ":generated");
  EXPECT_EQ(left->to_string(), ":generated");
  EXPECT_EQ(right->to_string(), ":generated");
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-95-theme "
                         ":windows-95-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            7);

  auto copy_ops = std::count_if(render_target()->render_ops.begin(),
                                render_target()->render_ops.end(),
                                [](const auto& op)
                                { return op.type == RenderOpType::RENDER_COPY; });
  EXPECT_GE(copy_ops, 4);
}

TEST_F(ScrollbarTest, windows_3_scrollbar_buttons_use_generated_theme_symbols)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/scrollbar
                   :style {:width 60}
                   :state {:axis :x :content-size 100 :value 0}}
                  {:mode 'ui/scrollbar
                   :style {:height 60}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  runtime.eval(R"(
    (defun resource-size [bundle id]
      (:size
       (head
        (filter (pixils.resource/list-images bundle)
                (fn [resource]
                  (= (:id resource) id))))))
  )");

  auto up = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-3-theme)
                           (fn [resource]
                             (= (:id resource) :windows-3-theme/scrollbar-arrow-up)))))
  )");
  auto down = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-3-theme)
                           (fn [resource]
                             (= (:id resource) :windows-3-theme/scrollbar-arrow-down)))))
  )");
  auto left = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-3-theme)
                           (fn [resource]
                             (= (:id resource) :windows-3-theme/scrollbar-arrow-left)))))
  )");
  auto right = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :windows-3-theme)
                           (fn [resource]
                             (= (:id resource) :windows-3-theme/scrollbar-arrow-right)))))
  )");

  ASSERT_NE(up, nullptr);
  ASSERT_NE(down, nullptr);
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(up->to_string(), ":generated");
  EXPECT_EQ(down->to_string(), ":generated");
  EXPECT_EQ(left->to_string(), ":generated");
  EXPECT_EQ(right->to_string(), ":generated");

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto horizontal_scrollbar = session.active_mode->children[0];
  auto vertical_scrollbar = session.active_mode->children[1];
  ASSERT_NE(horizontal_scrollbar, nullptr);
  ASSERT_NE(vertical_scrollbar, nullptr);
  ASSERT_EQ(horizontal_scrollbar->children.size(), 3u);
  ASSERT_EQ(vertical_scrollbar->children.size(), 3u);
  EXPECT_EQ(horizontal_scrollbar->children[0]->bounds.w, 15);
  EXPECT_EQ(horizontal_scrollbar->children[0]->bounds.h, 15);
  EXPECT_EQ(vertical_scrollbar->children[0]->bounds.w, 15);
  EXPECT_EQ(vertical_scrollbar->children[0]->bounds.h, 15);

  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:w (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :windows-3-theme "
                         ":windows-3-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            7);

  auto copy_ops = std::count_if(render_target()->render_ops.begin(),
                                render_target()->render_ops.end(),
                                [](const auto& op)
                                { return op.type == RenderOpType::RENDER_COPY; });
  EXPECT_GE(copy_ops, 4);

  auto centered_start_arrow =
    std::any_of(render_target()->render_ops.begin(),
                render_target()->render_ops.end(),
                [](const auto& op) {
                  return op.type == RenderOpType::RENDER_COPY &&
                         op.rendered_rect.x == 3 &&
                         op.rendered_rect.y == 3 &&
                         op.rendered_rect.w == 7 &&
                         op.rendered_rect.h == 7;
  });
  EXPECT_TRUE(centered_start_arrow);
}

TEST_F(ScrollbarTest, classic_blue_scrollbar_buttons_use_generated_theme_symbols)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/classic-blue
       :children [{:mode 'ui/scrollbar
                   :style {:width 50 :height 14}
                   :state {:axis :x :content-size 100 :value 0}}
                  {:mode 'ui/scrollbar
                   :style {:width 14 :height 50}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();
  runtime.eval(R"(
    (defun resource-size [bundle id]
      (:size
       (head
        (filter (pixils.resource/list-images bundle)
                (fn [resource]
                  (= (:id resource) id))))))
  )");

  auto up = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :classic-blue-theme)
                           (fn [resource]
                             (= (:id resource) :classic-blue-theme/scrollbar-arrow-up)))))
  )");
  auto down = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :classic-blue-theme)
                           (fn [resource]
                             (= (:id resource) :classic-blue-theme/scrollbar-arrow-down)))))
  )");
  auto left = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :classic-blue-theme)
                           (fn [resource]
                             (= (:id resource) :classic-blue-theme/scrollbar-arrow-left)))))
  )");
  auto right = runtime.eval(R"(
    (:source (head (filter (pixils.resource/list-images :classic-blue-theme)
                           (fn [resource]
                             (= (:id resource) :classic-blue-theme/scrollbar-arrow-right)))))
  )");

  ASSERT_NE(up, nullptr);
  ASSERT_NE(down, nullptr);
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(up->to_string(), ":generated");
  EXPECT_EQ(down->to_string(), ":generated");
  EXPECT_EQ(left->to_string(), ":generated");
  EXPECT_EQ(right->to_string(), ":generated");
  EXPECT_EQ(runtime.eval("(:w (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-up))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:w (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:h (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-down))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:w (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-left))")
              ->num()
              .get_int(),
            7);
  EXPECT_EQ(runtime.eval("(:w (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            4);
  EXPECT_EQ(runtime.eval("(:h (resource-size :classic-blue-theme "
                         ":classic-blue-theme/scrollbar-arrow-right))")
              ->num()
              .get_int(),
            7);

  auto copy_ops = std::count_if(render_target()->render_ops.begin(),
                                render_target()->render_ops.end(),
                                [](const auto& op)
                                { return op.type == RenderOpType::RENDER_COPY; });
  EXPECT_GE(copy_ops, 4);
}

TEST_F(ScrollbarTest, windows_3_scrollbar_handle_uses_fixed_theme_size)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :style {:layout {:direction :column}}
       :children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 400 :value 0}}
                  {:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 800 :value 0}}
                  {:mode 'ui/scrollbar
                   :style {:width 200}
                   :state {:axis :x :content-size 400 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 3u);
  auto short_content_scrollbar = session.active_mode->children[0];
  auto long_content_scrollbar = session.active_mode->children[1];
  auto horizontal_scrollbar = session.active_mode->children[2];
  ASSERT_NE(short_content_scrollbar, nullptr);
  ASSERT_NE(long_content_scrollbar, nullptr);
  ASSERT_NE(horizontal_scrollbar, nullptr);
  ASSERT_EQ(short_content_scrollbar->children.size(), 3u);
  ASSERT_EQ(long_content_scrollbar->children.size(), 3u);
  ASSERT_EQ(horizontal_scrollbar->children.size(), 3u);

  auto short_track = short_content_scrollbar->children[1];
  auto long_track = long_content_scrollbar->children[1];
  auto horizontal_track = horizontal_scrollbar->children[1];
  ASSERT_NE(short_track, nullptr);
  ASSERT_NE(long_track, nullptr);
  ASSERT_NE(horizontal_track, nullptr);
  ASSERT_EQ(short_track->children.size(), 1u);
  ASSERT_EQ(long_track->children.size(), 1u);
  ASSERT_EQ(horizontal_track->children.size(), 1u);
  EXPECT_GT(short_track->bounds.h, 100);
  EXPECT_GT(long_track->bounds.h, 100);
  EXPECT_GT(horizontal_track->bounds.w, 100);

  EXPECT_EQ(short_track->children[0]->bounds.h, 15);
  EXPECT_EQ(long_track->children[0]->bounds.h, 15);
  EXPECT_EQ(horizontal_track->children[0]->bounds.w, 15);
  EXPECT_EQ(short_track->children[0]->bounds.w, short_track->bounds.w);
  EXPECT_EQ(long_track->children[0]->bounds.w, long_track->bounds.w);
  EXPECT_EQ(horizontal_track->children[0]->bounds.h, horizontal_track->bounds.h);
}

TEST_F(ScrollbarTest, windows_3_scrollbar_handle_stays_fixed_without_scroll_range)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-3
       :children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto track = scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);
  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);

  EXPECT_GT(track->bounds.h, 100);
  EXPECT_EQ(handle->bounds.h, 15);
  EXPECT_LT(handle->bounds.h, track->bounds.h);
  EXPECT_EQ(handle->bounds.w, track->bounds.w);
}

TEST_F(ScrollbarTest, windows_95_scrollbar_handle_stays_proportional)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :style {:layout {:direction :row}}
       :children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 400 :value 0}}
                  {:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 800 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto short_content_scrollbar = session.active_mode->children[0];
  auto long_content_scrollbar = session.active_mode->children[1];
  ASSERT_NE(short_content_scrollbar, nullptr);
  ASSERT_NE(long_content_scrollbar, nullptr);
  ASSERT_EQ(short_content_scrollbar->children.size(), 3u);
  ASSERT_EQ(long_content_scrollbar->children.size(), 3u);

  auto short_track = short_content_scrollbar->children[1];
  auto long_track = long_content_scrollbar->children[1];
  ASSERT_NE(short_track, nullptr);
  ASSERT_NE(long_track, nullptr);
  ASSERT_EQ(short_track->children.size(), 1u);
  ASSERT_EQ(long_track->children.size(), 1u);
  EXPECT_GT(short_track->bounds.h, 100);
  EXPECT_GT(long_track->bounds.h, 100);

  EXPECT_GT(short_track->children[0]->bounds.h, long_track->children[0]->bounds.h);
  EXPECT_EQ(short_track->children[0]->bounds.w, short_track->bounds.w);
  EXPECT_EQ(long_track->children[0]->bounds.w, long_track->bounds.w);
}

TEST_F(ScrollbarTest, windows_95_scrollbar_handle_fills_track_without_scroll_range)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:theme 'pixils/windows-95
       :children [{:mode 'ui/scrollbar
                   :style {:height 200}
                   :state {:axis :y :content-size 100 :value 0}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto scrollbar = session.active_mode->children[0];
  ASSERT_NE(scrollbar, nullptr);
  ASSERT_EQ(scrollbar->children.size(), 3u);

  auto track = scrollbar->children[1];
  ASSERT_NE(track, nullptr);
  ASSERT_EQ(track->children.size(), 1u);
  auto handle = track->children[0];
  ASSERT_NE(handle, nullptr);

  EXPECT_GT(handle->bounds.h, 100);
  EXPECT_EQ(handle->bounds.w, track->bounds.w);
}
