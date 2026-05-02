
#include "../render_fixture.h"

#include <gtest/gtest.h>
#include <sdl2_mock/mock_resources.h>

class RenderTest : public RenderFixture
{
};

TEST_F(RenderTest, rect_accepts_map_style_points)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/rect!
                  {:x 5 :y 10}
                  {:x 25 :y 30}
                  {:fill true}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].rendered_rect.x, 5);
  EXPECT_EQ(ops[0].rendered_rect.y, 10);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);
}

TEST_F(RenderTest, rect_accepts_inline_color_map_in_options)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/rect!
                  {:x 0 :y 0}
                  {:x 10 :y 10}
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_EQ(render_target()->render_ops.size(), 1u);
}

TEST_F(RenderTest, image_accepts_rotation_in_radians)
{
  // Given
  SDLMock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:pos {:x 12 :y 18}
                   :rotation 1.5707963}))
    })
  )");
  session.push_mode("test-mode", Lisple::Constant::NIL);

  // When
  ASSERT_NO_THROW(session.render_mode());

  // Then
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 8);
  EXPECT_NEAR(ops[0].rotation_degrees, 90.0, 0.01);
}
