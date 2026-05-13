#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using ScrollbarTest = RenderFixture;

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

  auto value =
    Lisple::Dict::get_property(scrollbar->state, Lisple::RTValue::keyword("value"));
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(value->num().get_int(), 5);
}
