
#include "../render_fixture.h"

#include <gtest/gtest.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

using FrameCycleTest = RenderFixture;

TEST_F(FrameCycleTest, init_hook_sets_initial_state)
{
  // Given
  runtime.eval("(pixils/defmode test-mode {:init (fn [state ctx] {:x 42})})");

  // When
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // Then
  auto state = session.active_mode.state;
  auto x = Lisple::Dict::get_property(state, Lisple::RTValue::keyword("x"));
  EXPECT_EQ(x->num().get_int(), 42);
}

TEST_F(FrameCycleTest, update_hook_receives_and_returns_new_state)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :init   (fn [state ctx] {:count 0})
      :update (fn [state events ctx] (prn "update!") (assoc state :count (+ (:count state) 1)))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When
  session.update_mode();

  // Then
  auto state = session.active_mode.state;
  auto count = Lisple::Dict::get_property(state, Lisple::RTValue::keyword("count"));
  EXPECT_EQ(count->num().get_int(), 1);
}

TEST_F(FrameCycleTest, render_hook_produces_fill_rect_draw_op)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/rect!
                  (pixils.point/point 10 20)
                  (pixils.point/point 50 60)
                  {:fill true}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].rendered_rect.x, 10);
  EXPECT_EQ(ops[0].rendered_rect.y, 20);
  EXPECT_EQ(ops[0].rendered_rect.w, 40);
  EXPECT_EQ(ops[0].rendered_rect.h, 40);
}

TEST_F(FrameCycleTest, render_hook_receives_render_context_with_buffer_dimensions)
{
  // Given - render_ctx.buffer_dim is {320, 200} per RenderFixture
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (let [dim (:buffer-size ctx)]
                  (pixils.render/rect!
                    (pixils.point/point 0 0)
                    (pixils.point/point (:w dim) (:h dim))
                    {:fill true})))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When
  session.render_mode();

  // Then
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].rendered_rect.w, 320);
  EXPECT_EQ(ops[0].rendered_rect.h, 200);
}

TEST_F(FrameCycleTest, full_frame_cycle_init_update_render)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :init   (fn [state ctx] {:ticks 0})
      :update (fn [state events ctx] (assoc state :ticks (+ (:ticks state) 1)))
      :render (fn [state ctx]
                (pixils.render/rect!
                  (pixils.point/point 0 0)
                  (pixils.point/point (:ticks state) (:ticks state))
                  {:fill true}))
    })
  )");

  // When
  session.push_mode("test-mode", Lisple::Constant::NIL);
  session.update_mode();
  session.render_mode();

  // Then
  auto count =
    Lisple::Dict::get_property(session.active_mode.state, Lisple::RTValue::keyword("ticks"));
  EXPECT_EQ(count->num().get_int(), 1);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].rendered_rect.w, 1);
  EXPECT_EQ(ops[0].rendered_rect.h, 1);
}
