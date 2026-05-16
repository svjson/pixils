#include "../../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using IconTest = RenderFixture;

TEST_F(IconTest, plain_icon_is_not_draggable_or_absolute_positioned)
{
  auto modes = runtime.lookup("pixils/modes");
  ASSERT_NE(modes, nullptr);

  auto icon_mode_value = Lisple::Dict::get_property(modes, Lisple::symbol("ui/icon"));
  ASSERT_NE(icon_mode_value, nullptr);
  const auto& icon_mode = Lisple::obj<Pixils::Runtime::Mode>(*icon_mode_value);
  EXPECT_FALSE(icon_mode.drag.has_value());

  runtime.eval(R"(
    (pixils/defmode root-mode
      {:style {:width 100
               :height 20
               :layout {:direction :row}}
       :children [{:mode 'ui/icon
                   :style {:width 20
                           :height 20}
                   :state {:item {:id :one}}}
                  {:style {:width 20
                           :height 20}}]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 2u);
  auto icon = session.active_mode->children[0];
  auto sibling = session.active_mode->children[1];
  ASSERT_NE(icon, nullptr);
  ASSERT_NE(sibling, nullptr);
  EXPECT_EQ(icon->bounds.x, 0);
  EXPECT_EQ(sibling->bounds.x, 20);
}

TEST_F(IconTest, make_draggable_adds_drag_policy_to_arbitrary_child)
{
  runtime.eval(R"(
    (pixils/defmode root-mode
      {:init (fn [state ctx] {:start nil :move nil :end nil})
       :style {:width 100
               :height 40}
       :on {:test/drag-start (fn [state event ctx]
                               (assoc state :start (:payload event)))
            :test/drag-move (fn [state event ctx]
                              (assoc state :move (:payload event)))
            :test/drag-end (fn [state event ctx]
                             (assoc state :end true))}
       :children [(pixils.ui/make-draggable
                   {:style {:width 30
                            :height 20}
                    :state {:item {:id :disk}}}
                   {:threshold 4
                    :start-event :test/drag-start
                    :move-event :test/drag-move
                    :end-event :test/drag-end})]})
  )");

  session.push_mode("root-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  ASSERT_EQ(session.active_mode->children.size(), 1u);
  auto child = session.active_mode->children[0];
  ASSERT_NE(child, nullptr);
  ASSERT_NE(child->mode, nullptr);
  ASSERT_TRUE(child->mode->drag.has_value());
  ASSERT_NE(child->mode->on_drag_start, nullptr);
  ASSERT_NE(child->mode->on_drag, nullptr);
  ASSERT_NE(child->mode->on_drag_end, nullptr);
  ASSERT_NE(child->mode->on_drag_start->type, Lisple::Value::Type::NIL);
  ASSERT_NE(child->mode->on_drag->type, Lisple::Value::Type::NIL);
  ASSERT_NE(child->mode->on_drag_end->type, Lisple::Value::Type::NIL);
  ASSERT_EQ(child->bounds.x, 0);
  ASSERT_EQ(child->bounds.y, 0);
  ASSERT_EQ(child->bounds.w, 30);
  ASSERT_EQ(child->bounds.h, 20);

  input().mouse_down({5, 5});
  update_cycle();

  input().mouse_move({8, 5});
  update_cycle();
  auto start =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("start"));
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->type, Lisple::Value::Type::NIL);

  input().mouse_move({12, 5});
  update_cycle();
  input().mouse_move({15, 8});
  update_cycle();
  input().mouse_up({15, 8});
  update_cycle();
  update_cycle();

  start = Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("start"));
  auto move =
    Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("move"));
  auto end = Lisple::Dict::get_property(session.active_mode->state, Lisple::keyword("end"));
  ASSERT_NE(start, nullptr);
  ASSERT_NE(move, nullptr);
  ASSERT_NE(end, nullptr);
  EXPECT_EQ(start->to_string(),
            "{:item {:id :disk} :offset {:x 5 :y 5} :position {:x 7 :y 0}}");
  EXPECT_EQ(move->to_string(), "{:position {:x 15 :y 8}}");
  EXPECT_EQ(end->to_string(), "true");
}
