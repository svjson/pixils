
#include "../render_fixture.h"
#include <pixils/font_registry.h>
#include <pixils/text.h>

#include <algorithm>
#include <gtest/gtest.h>
#include <roo/runtime/dict.h>
#include <sdl3_mock/mock_resources.h>
#include <vector>

class RenderTest : public RenderFixture
{
};

namespace
{
  bool has_fill_rect(const std::vector<RenderOperation>& ops, const SDL_Rect& rect)
  {
    for (const auto& op : ops)
    {
      if (op.type == RenderOpType::FILL_RECT && op.rendered_rect.x == rect.x &&
          op.rendered_rect.y == rect.y && op.rendered_rect.w == rect.w &&
          op.rendered_rect.h == rect.h)
      {
        return true;
      }
    }
    return false;
  }

  bool has_filled_pixel(const std::vector<RenderOperation>& ops, int x, int y)
  {
    for (const auto& op : ops)
    {
      if (op.type == RenderOpType::FILL_RECT && x >= op.rendered_rect.x &&
          x < op.rendered_rect.x + op.rendered_rect.w && y >= op.rendered_rect.y &&
          y < op.rendered_rect.y + op.rendered_rect.h)
      {
        return true;
      }
    }
    return false;
  }

  size_t fill_rect_count(const std::vector<RenderOperation>& ops)
  {
    return static_cast<size_t>(std::count_if(
      ops.begin(),
      ops.end(),
      [](const RenderOperation& op) { return op.type == RenderOpType::FILL_RECT; }));
  }
} // namespace

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
  session.push_mode("test-mode", Roo::Constant::NIL);

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
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_EQ(render_target()->render_ops.size(), 1u);
}

TEST_F(RenderTest, rect_outline_draws_edges_inside_rect_bounds)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/rect!
                  {:x 2 :y 3 :w 4 :h 5}
                  {:color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 4u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 3, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 7, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 3, 1, 5}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{5, 3, 1, 5}));
}

TEST_F(RenderTest, line_accepts_stroke_width_option)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/line!
                  {:x 2 :y 5}
                  {:x 6 :y 5}
                  {:stroke-width 3 :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 4, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 5, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 6, 5, 1}));
}

TEST_F(RenderTest, vertical_line_stroke_uses_pixel_centered_width)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/line!
                  {:x 5 :y 2}
                  {:x 5 :y 4}
                  {:stroke-width 2 :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{4, 2, 2, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{4, 3, 2, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{4, 4, 2, 1}));
}

TEST_F(RenderTest, line_still_accepts_color_as_third_argument)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/line!
                  {:x 2 :y 5}
                  {:x 6 :y 5}
                  {:r 200 :g 0 :b 0}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
}

TEST_F(RenderTest, circle_fill_draws_horizontal_scanlines)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/circle!
                  {:x 10 :y 10 :r 2}
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 5u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 8, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{9, 9, 3, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{8, 10, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{9, 11, 3, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 12, 1, 1}));
}

TEST_F(RenderTest, circle_outline_draws_perimeter_pixels)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/circle!
                  {:x 10 :y 10 :r 2}
                  {:color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 8, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 12, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{8, 10, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{12, 10, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{10, 10, 1, 1}));
}

TEST_F(RenderTest, circle_smooth_fill_draws_coverage_pixels)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/circle!
                  {:x 10 :y 10 :r 2}
                  {:fill true
                   :rasterization :smooth
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_EQ(fill_rect_count(ops), 21u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 10, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{12, 10, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{12, 12, 1, 1}));
}

TEST_F(RenderTest, circle_smooth_outline_draws_antialiased_neighbors)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/circle!
                  {:x 10 :y 10 :r 2}
                  {:rasterization :smooth
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 8, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{9, 9, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{10, 10, 1, 1}));
}

TEST_F(RenderTest, circle_fill_accepts_solid_fill_style)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/circle!
                  {:x 10 :y 10 :r 2}
                  {:fill true
                   :fill-style {:type :solid
                                :color {:r 200 :g 0 :b 0}}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_FALSE(render_target()->render_ops.empty());
}

TEST_F(RenderTest, ellipse_fill_draws_horizontal_scanlines)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/ellipse!
                  {:x 10 :y 10 :rx 3 :ry 2}
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 5u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 8, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{8, 9, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{7, 10, 7, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{8, 11, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 12, 1, 1}));
}

TEST_F(RenderTest, ellipse_outline_draws_perimeter_pixels)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/ellipse!
                  {:x 10 :y 10 :rx 3 :ry 2}
                  {:color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 8, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 12, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{7, 10, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{13, 10, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{10, 10, 1, 1}));
}

TEST_F(RenderTest, ellipse_smooth_fill_draws_coverage_pixels)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/ellipse!
                  {:x 10 :y 10 :rx 3 :ry 2}
                  {:fill true
                   :rasterization :smooth
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(fill_rect_count(ops) > 5u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{10, 10, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{13, 10, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{13, 12, 1, 1}));
}

TEST_F(RenderTest, ellipse_smooth_outline_draws_antialiased_neighbors)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/ellipse!
                  {:x 10 :y 10 :rx 3 :ry 2}
                  {:rasterization :smooth
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{13, 10, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{12, 9, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{10, 10, 1, 1}));
}

TEST_F(RenderTest, ellipse_fill_accepts_solid_fill_style)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/ellipse!
                  {:x 10 :y 10 :rx 3 :ry 2}
                  {:fill true
                   :fill-style {:type :solid
                                :color {:r 200 :g 0 :b 0}}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_FALSE(render_target()->render_ops.empty());
}

TEST_F(RenderTest, circle_rejects_unknown_rasterization)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/circle!
                  {:x 10 :y 10 :r 2}
                  {:rasterization :wat}))})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  EXPECT_THROW(session.render_mode(), Roo::TypeError);
}

TEST_F(RenderTest, ellipse_with_negative_radius_draws_nothing)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/ellipse!
                  {:x 10 :y 10 :rx -1 :ry 2}
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_TRUE(render_target()->render_ops.empty());
}

TEST_F(RenderTest, polygon_fill_draws_horizontal_scanlines)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 2} {:x 6 :y 2} {:x 6 :y 5} {:x 2 :y 5}]
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 2, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 3, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 4, 4, 1}));
}

TEST_F(RenderTest, polygon_fill_uses_pixel_centers_for_fractional_edges)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2.1 :y 1} {:x 5.2 :y 1} {:x 5.2 :y 3} {:x 2.1 :y 3}]
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 1, 3, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 2, 3, 1}));
}

TEST_F(RenderTest, polygon_accepts_erase_alpha_blend_mode)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 2} {:x 6 :y 2} {:x 6 :y 5} {:x 2 :y 5}]
                  {:fill true
                   :color {:r 255 :g 255 :b 255 :a 255}
                   :blend-mode :erase-alpha
                   :rasterization :pixel}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 2, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 3, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 4, 4, 1}));
}

TEST_F(RenderTest, polygon_rejects_unknown_blend_mode)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 2} {:x 6 :y 2} {:x 6 :y 5} {:x 2 :y 5}]
                  {:fill true
                   :color {:r 255 :g 255 :b 255 :a 255}
                   :blend-mode :screen}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  EXPECT_THROW(session.render_mode(), Roo::TypeError);
}

TEST_F(RenderTest, polygon_fill_handles_concave_shapes)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 0 :y 0}
                   {:x 4 :y 0}
                   {:x 4 :y 2}
                   {:x 2 :y 2}
                   {:x 2 :y 4}
                   {:x 0 :y 4}]
                  {:fill true :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 4u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 0, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 1, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 2, 2, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 3, 2, 1}));
}

TEST_F(RenderTest, polygon_smooth_fill_uses_existing_scanline_fill)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 2.1} {:x 6 :y 2.1} {:x 6 :y 2.4} {:x 2 :y 2.4}]
                  {:fill true
                   :rasterization :smooth
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 0u);
}

TEST_F(RenderTest, polygon_smooth_outline_draws_antialiased_neighbors)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 2.2} {:x 6 :y 2.2}]
                  {:rasterization :smooth
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 1, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 2, 1, 1}));
}

TEST_F(RenderTest, polygon_fill_accepts_vertex_color_fill_style_for_triangle)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 0 :y 0} {:x 2 :y 0} {:x 0 :y 2}]
                  {:fill true
                   :fill-style {:type :vertex-colors
                                :colors [{:r 255 :g 0 :b 0}
                                         {:r 0 :g 255 :b 0}
                                         {:r 0 :g 0 :b 255}]}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 0, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{1, 0, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 1, 1, 1}));
}

TEST_F(RenderTest, polygon_fill_accepts_vertex_color_fill_style_for_quad)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 0 :y 0} {:x 2 :y 0} {:x 2 :y 2} {:x 0 :y 2}]
                  {:fill true
                   :fill-style {:type :vertex-colors
                                :colors [{:r 255 :g 0 :b 0}
                                         {:r 0 :g 255 :b 0}
                                         {:r 0 :g 0 :b 255}
                                         {:r 255 :g 255 :b 255}]}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 4u);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 0, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{1, 0, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 1, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{1, 1, 1, 1}));
}

TEST_F(RenderTest, polygon_fill_triangulates_concave_vertex_color_fill_style)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 0 :y 0}
                   {:x 4 :y 0}
                   {:x 4 :y 2}
                   {:x 2 :y 2}
                   {:x 2 :y 4}
                   {:x 0 :y 4}]
                  {:fill true
                   :fill-style {:type :vertex-colors
                                :colors [{:r 255 :g 0 :b 0}
                                         {:r 0 :g 255 :b 0}
                                         {:r 0 :g 0 :b 255}
                                         {:r 255 :g 255 :b 0}
                                         {:r 255 :g 0 :b 255}
                                         {:r 0 :g 255 :b 255}]}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 0, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{3, 1, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 3, 1, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{3, 3, 1, 1}));
}

TEST_F(RenderTest, polygon_vertex_color_fill_ignores_rasterization_option)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 0 :y 0} {:x 2 :y 0} {:x 0 :y 2}]
                  {:fill true
                   :rasterization :smooth
                   :fill-style {:type :vertex-colors
                                :colors [{:r 255 :g 0 :b 0}
                                         {:r 0 :g 255 :b 0}
                                         {:r 0 :g 0 :b 255}]}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 0, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{1, 0, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 1, 1, 1}));
}

TEST_F(RenderTest, polygon_vertex_color_fill_style_requires_one_color_per_point)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 0 :y 0} {:x 2 :y 0} {:x 0 :y 2}]
                  {:fill true
                   :fill-style {:type :vertex-colors
                                :colors [{:r 255 :g 0 :b 0}
                                         {:r 0 :g 255 :b 0}]}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  EXPECT_THROW(session.render_mode(), Roo::TypeError);
}

TEST_F(RenderTest, polygon_outline_accepts_stroke_width)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 2} {:x 6 :y 2} {:x 6 :y 5} {:x 2 :y 5}]
                  {:close true :stroke-width 2 :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 1, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 2, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{5, 2, 2, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{1, 2, 2, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 5, 5, 1}));
}

TEST_F(RenderTest, polygon_stroke_defaults_to_miter_line_join)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 6} {:x 6 :y 6} {:x 6 :y 2}]
                  {:stroke-width 4 :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_TRUE(has_filled_pixel(render_target()->render_ops, 8, 8));
}

TEST_F(RenderTest, polygon_stroke_accepts_bevel_line_join)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 6} {:x 6 :y 6} {:x 6 :y 2}]
                  {:stroke-width 4
                   :line-join :bevel
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_FALSE(has_filled_pixel(render_target()->render_ops, 8, 8));
}

TEST_F(RenderTest, polygon_stroke_accepts_round_line_join)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 6} {:x 6 :y 6} {:x 6 :y 2}]
                  {:stroke-width 4
                   :line-join :round
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_filled_pixel(ops, 7, 7));
  EXPECT_FALSE(has_filled_pixel(ops, 8, 8));
}

TEST_F(RenderTest, polygon_stroke_accepts_none_line_join_for_independent_segments)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 6} {:x 6 :y 6} {:x 6 :y 2}]
                  {:stroke-width 4
                   :line-join :none
                   :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  EXPECT_FALSE(has_filled_pixel(render_target()->render_ops, 8, 8));
}

TEST_F(RenderTest, polygon_rejects_unknown_line_join)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 6} {:x 6 :y 6} {:x 6 :y 2}]
                  {:stroke-width 4 :line-join :pointy}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  EXPECT_THROW(session.render_mode(), Roo::TypeError);
}

TEST_F(RenderTest, polygon_rejects_unknown_rasterization)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 6} {:x 6 :y 6} {:x 6 :y 2}]
                  {:rasterization :wat}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  EXPECT_THROW(session.render_mode(), Roo::TypeError);
}

TEST_F(RenderTest, filled_polygon_strokes_only_when_stroke_width_is_explicit)
{
  // Given
  runtime.eval(R"(
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/polygon!
                  [{:x 2 :y 2} {:x 6 :y 2} {:x 6 :y 5} {:x 2 :y 5}]
                  {:fill true :stroke-width 2 :color {:r 200 :g 0 :b 0}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  // When / Then
  ASSERT_NO_THROW(session.render_mode());
  auto& ops = render_target()->render_ops;
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 2, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 3, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 4, 4, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 1, 5, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{2, 5, 5, 1}));
}

TEST_F(RenderTest, with_clip_rect_restores_previous_clip)
{
  // Given
  render_ctx.set_clip_rect(Pixils::Rect{1, 2, 30, 40});

  // When
  ASSERT_NO_THROW(runtime.eval(R"(
    (pixils.render/with-clip-rect {:x 4 :y 5 :w 6 :h 7}
      (pixils.render/rect! {:x 0 :y 0 :w 20 :h 20} {:fill true}))
  )"));

  // Then
  ASSERT_TRUE(render_ctx.current_clip_rect.has_value());
  EXPECT_EQ(*render_ctx.current_clip_rect, (Pixils::Rect{1, 2, 30, 40}));
}

TEST_F(RenderTest, with_clip_rect_restores_clip_after_body_error)
{
  // Given
  render_ctx.set_clip_rect(Pixils::Rect{1, 2, 30, 40});

  // When
  EXPECT_THROW(runtime.eval(R"(
    (pixils.render/with-clip-rect {:x 4 :y 5 :w 6 :h 7}
      (/ 1 0))
  )"),
               Roo::RooException);

  // Then
  ASSERT_TRUE(render_ctx.current_clip_rect.has_value());
  EXPECT_EQ(*render_ctx.current_clip_rect, (Pixils::Rect{1, 2, 30, 40}));
}

TEST_F(RenderTest, image_accepts_rotation_in_radians)
{
  // Given
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
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
  session.push_mode("test-mode", Roo::Constant::NIL);

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

TEST_F(RenderTest, image_accepts_source_rect)
{
  SDL3Mock::prepared_surfaces["./tiles.png"] = {32, 16};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:tiles "tiles.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/tiles
                  {:pos {:x 3 :y 4}
                   :source {:x 16 :y 0 :w 8 :h 8}
                   :scale 2}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 3);
  EXPECT_EQ(ops[0].rendered_rect.y, 4);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 16);
}

TEST_F(RenderTest, images_draws_supported_entries_as_single_geometry_operation)
{
  SDL3Mock::prepared_surfaces["./tiles.png"] = {32, 16};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:tiles "tiles.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/images!
                  :sprites/tiles
                  [{:pos {:x 3 :y 4}
                    :source {:x 16 :y 0 :w 8 :h 8}
                    :scale 2}
                   {:target {:x 30 :y 10 :w 8 :h 12}
                    :source {:x 24 :y 0 :w 8 :h 8}}]))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_GEOMETRY);
  EXPECT_EQ(ops[0].rendered_rect.x, 3);
  EXPECT_EQ(ops[0].rendered_rect.y, 4);
  EXPECT_EQ(ops[0].rendered_rect.w, 35);
  EXPECT_EQ(ops[0].rendered_rect.h, 18);
  EXPECT_EQ(ops[0].vertex_count, 8);
  EXPECT_EQ(ops[0].index_count, 12);
}

TEST_F(RenderTest, images_falls_back_to_image_rendering_for_rotated_entries)
{
  SDL3Mock::prepared_surfaces["./tiles.png"] = {32, 16};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:tiles "tiles.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/images!
                  :sprites/tiles
                  [{:pos {:x 1 :y 2}
                    :source {:x 0 :y 0 :w 8 :h 8}}
                   {:pos {:x 10 :y 20}
                    :source {:x 8 :y 0 :w 8 :h 8}
                    :rotation 1.5707963}
                   {:pos {:x 30 :y 40}
                    :source {:x 16 :y 0 :w 8 :h 8}}]))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_GEOMETRY);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.x, 10);
  EXPECT_EQ(ops[1].rendered_rect.y, 20);
  EXPECT_EQ(ops[1].rendered_rect.w, 8);
  EXPECT_EQ(ops[1].rendered_rect.h, 8);
  EXPECT_NEAR(ops[1].rotation_degrees, 90.0, 0.01);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_GEOMETRY);
}

TEST_F(RenderTest, image_accepts_target_point)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:target {:x 12 :y 18}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 8);
}

TEST_F(RenderTest, image_target_rect_scales_single_copy)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:target {:x 2 :y 3 :w 40 :h 20}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 2);
  EXPECT_EQ(ops[0].rendered_rect.y, 3);
  EXPECT_EQ(ops[0].rendered_rect.w, 40);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);
}

TEST_F(RenderTest, image_accepts_direct_target_rect_map)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:x 2 :y 3 :w 40 :h 20}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 2);
  EXPECT_EQ(ops[0].rendered_rect.y, 3);
  EXPECT_EQ(ops[0].rendered_rect.w, 40);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);
}

TEST_F(RenderTest, image_rejects_direct_point_map_with_options)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  EXPECT_THROW(runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils.render/image!
      :sprites/ship
      {:x 2 :y 3 :scale 2})
  )"),
               Roo::RooException);
}

TEST_F(RenderTest, image_clip_rect_restores_previous_clip)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  render_ctx.set_clip_rect(Pixils::Rect{1, 2, 30, 40});
  ASSERT_NO_THROW(runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils.render/image!
      :sprites/ship
      {:target {:x 12 :y 18}
       :clip-rect {:x 4 :y 5 :w 6 :h 7}})
  )"));

  ASSERT_TRUE(render_ctx.current_clip_rect.has_value());
  EXPECT_EQ(*render_ctx.current_clip_rect, (Pixils::Rect{1, 2, 30, 40}));
}

TEST_F(RenderTest, image_repeat_fills_clip_rect_from_target_anchor)
{
  SDL3Mock::prepared_surfaces["./tile.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:tile "tile.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/tile
                  {:target {:x 14 :y 10}
                   :clip-rect {:x 10 :y 10 :w 20 :h 16}
                   :repeat-x? true
                   :repeat-y? true}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 6u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 6);
  EXPECT_EQ(ops[0].rendered_rect.y, 10);
  EXPECT_EQ(ops[1].rendered_rect.x, 14);
  EXPECT_EQ(ops[1].rendered_rect.y, 10);
  EXPECT_EQ(ops[2].rendered_rect.x, 22);
  EXPECT_EQ(ops[2].rendered_rect.y, 10);
  EXPECT_EQ(ops[3].rendered_rect.x, 6);
  EXPECT_EQ(ops[3].rendered_rect.y, 18);
}

TEST_F(RenderTest, image_repeat_uses_active_clip_as_default_bounds)
{
  SDL3Mock::prepared_surfaces["./tile.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:tile "tile.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/with-clip-rect {:x 10 :y 10 :w 20 :h 16}
                  (pixils.render/image!
                    :sprites/tile
                    {:target {:x 14 :y 10}
                     :repeat-x? true
                     :repeat-y? true})))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 6u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 6);
  EXPECT_EQ(ops[0].rendered_rect.y, 10);
  EXPECT_EQ(ops[5].rendered_rect.x, 22);
  EXPECT_EQ(ops[5].rendered_rect.y, 18);
}

TEST_F(RenderTest, image_accepts_flip_options)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:pos {:x 12 :y 18}
                   :flip-x? true
                   :flip-y? true}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 8);
}

TEST_F(RenderTest, image_accepts_opacity)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:pos {:x 12 :y 18}
                   :opacity 0.5}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
}

TEST_F(RenderTest, image_accepts_erase_alpha_blend_mode)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :sprites/ship
                  {:pos {:x 12 :y 18}
                   :blend-mode :erase-alpha}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 8);
}

TEST_F(RenderTest, image_rejects_unknown_blend_mode)
{
  SDL3Mock::prepared_surfaces["./ship.png"] = {16, 8};
  EXPECT_THROW(runtime.eval(R"(
    (pixils/defbundle sprites {:images {:ship "ship.png"}})
    (pixils.render/image!
      :sprites/ship
      {:pos {:x 12 :y 18}
       :blend-mode :screen})
  )"),
               Roo::RooException);
}

TEST_F(RenderTest, generated_image_can_draw_base_and_apply_alpha_masks)
{
  SDL3Mock::prepared_surfaces["./base.png"] = {16, 8};
  SDL3Mock::prepared_surfaces["./mask-a.png"] = {16, 8};
  SDL3Mock::prepared_surfaces["./mask-b.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle sprites
      {:images {:base "base.png"
                :mask-a "mask-a.png"
                :mask-b "mask-b.png"}})
    (pixils.resource/create-bundle! :generated)
    (pixils.resource/create-image!
      :generated/composite
      {:size {:w 16 :h 8}}
      (fn []
        (pixils.render/image!
          :sprites/base
          {:target {:x 0 :y 0 :w 16 :h 8}})
        (pixils.render/image!
          :sprites/mask-a
          {:target {:x 0 :y 0 :w 16 :h 8}
           :blend-mode :erase-alpha})
        (pixils.render/image!
          :sprites/mask-b
          {:target {:x 0 :y 0 :w 16 :h 8}
           :blend-mode :erase-alpha})))
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :generated/composite
                  {:pos {:x 4 :y 5}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 4);
  EXPECT_EQ(ops[0].rendered_rect.y, 5);
  EXPECT_EQ(ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].rendered_rect.h, 8);
  ASSERT_EQ(ops[0].sub_ops.size(), 3u);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].sub_ops[1].rendered_rect.w, 16);
  EXPECT_EQ(ops[0].sub_ops[2].rendered_rect.w, 16);
}

TEST_F(RenderTest, image_missing_asset_is_noop)
{
  runtime.eval(R"(
    (pixils/defbundle-dynamic project-assets)
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/image!
                  :project-assets/missing
                  {:pos {:x 12 :y 18}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_TRUE(render_target()->render_ops.empty());
}

TEST_F(RenderTest, style_background_image_renders_once_without_repeat)
{
  SDL3Mock::prepared_surfaces["./checkmark.png"] = {7, 7};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:checkmark "checkmark.png"}})
    (pixils/defmode test-mode
      {:style {:background {:image :icons/checkmark}}})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 7);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
}

TEST_F(RenderTest, StyleBackgroundImageRepeatsWithinBounds)
{
  SDL3Mock::prepared_surfaces["./checkmark.png"] = {7, 7};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:checkmark "checkmark.png"}})
    (pixils/defmode test-mode
      {:style {:width 15
               :height 12
               :background {:image :icons/checkmark
                            :repeat-x? true
                            :repeat-y? true}}})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 6u);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[1].rendered_rect.x, 7);
  EXPECT_EQ(ops[1].rendered_rect.y, 0);
  EXPECT_EQ(ops[2].rendered_rect.x, 14);
  EXPECT_EQ(ops[2].rendered_rect.y, 0);
  EXPECT_EQ(ops[3].rendered_rect.x, 0);
  EXPECT_EQ(ops[3].rendered_rect.y, 7);
}

TEST_F(RenderTest, style_background_image_accepts_opacity)
{
  SDL3Mock::prepared_surfaces["./checkmark.png"] = {7, 7};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:checkmark "checkmark.png"}})
    (pixils/defmode test-mode
      {:style {:background {:image :icons/checkmark
                            :opacity 0.5}}})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.w, 7);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
}

TEST_F(RenderTest, style_background_image_can_fit_source_and_align)
{
  SDL3Mock::prepared_surfaces["./icons.png"] = {32, 32};
  runtime.eval(R"(
    (pixils/defbundle icons {:images {:sheet "icons.png"}})
    (pixils/defmode test-mode
      {:style {:width 20
               :height 20
               :background {:image :icons/sheet
                            :source {:x 8 :y 4 :w 8 :h 4}
                            :fit :contain
                            :align :center}}})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 5);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 10);
}

TEST_F(RenderTest, scaled_root_centers_fill_child_background_in_logical_area)
{
  SDL3Mock::prepared_surfaces["./logo.png"] = {20, 10};
  runtime.eval(R"(
    (pixils/defbundle title {:images {:logo "logo.png"}})
    (pixils/defmode test-mode
      {:style {:scale 2}
       :children [{:style {:background {:image :title/logo
                                        :align :center}
                           :width :fill
                           :height :fill}}]})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 320);
  EXPECT_EQ(ops[0].rendered_rect.h, 200);

  ASSERT_EQ(ops[0].sub_ops.size(), 1u);
  EXPECT_EQ(ops[0].sub_ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.x, 70);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.y, 45);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.h, 10);
}

TEST_F(RenderTest, style_corner_radius_rounds_background_fill)
{
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:style {:width 10
               :height 6
               :background {:r 200 :g 0 :b 0}
               :corner-radius 3}})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 6u);
  EXPECT_EQ(ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].rendered_rect.x, 1);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 8);
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 2, 10, 1}));
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{0, 0, 10, 1}));
}

TEST_F(RenderTest, style_directional_corner_radius_only_rounds_selected_corners)
{
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:style {:width 10
               :height 6
               :background {:r 200 :g 0 :b 0}
               :corner-radius {:tl 3 :tr 0 :br 3 :bl 0}}})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 6u);
  EXPECT_EQ(ops[0].rendered_rect.x, 1);
  EXPECT_EQ(ops[0].rendered_rect.w, 9);
  EXPECT_EQ(ops.back().rendered_rect.x, 0);
  EXPECT_EQ(ops.back().rendered_rect.w, 9);
}

TEST_F(RenderTest, style_corner_radius_rounds_border)
{
  runtime.eval(R"(
    (pixils/defmode test-mode
      {:style {:width 10
               :height 6
               :corner-radius 3
               :border {:thickness 1
                        :line-style :solid
                        :color {:r 0 :g 0 :b 0}}}})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_FALSE(ops.empty());
  EXPECT_EQ(ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].rendered_rect.x, 1);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 8);
  EXPECT_FALSE(has_fill_rect(ops, SDL_Rect{0, 0, 10, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{0, 2, 1, 1}));
  EXPECT_TRUE(has_fill_rect(ops, SDL_Rect{9, 2, 1, 1}));
}

TEST_F(RenderTest, absolute_positioned_children_render_after_flow_siblings)
{
  runtime.eval(R"(
    (pixils/defmode box {})
    (pixils/defmode test-mode
      {:children [{:mode 'box
                   :style {:position :absolute
                           :width 10
                           :height 10
                           :background {:r 200 :g 0 :b 0}}}
                  {:mode 'box
                   :style {:width 20
                           :height 20
                           :background {:r 0 :g 200 :b 0}}}]})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[1].rendered_rect.w, 10);
  EXPECT_EQ(ops[1].rendered_rect.h, 10);
}

TEST_F(RenderTest, scaled_view_renders_to_logical_texture_and_copies_to_scaled_footprint)
{
  runtime.eval(R"(
    (pixils/defmode scaled-panel
      {:style {:width 20 :height 10 :scale 2}
       :render (fn [state ctx]
                 (pixils.render/rect!
                   {:x 0 :y 0}
                   {:x 20 :y 10}
                   {:fill true}))})
  )");
  session.push_mode("scaled-panel", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 40);
  EXPECT_EQ(ops[0].rendered_rect.h, 20);

  ASSERT_EQ(ops[0].sub_ops.size(), 1u);
  EXPECT_EQ(ops[0].sub_ops[0].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].sub_ops[0].rendered_rect.h, 10);
}

TEST_F(RenderTest, translucent_view_renders_to_texture_and_copies_whole_subtree)
{
  runtime.eval(R"(
    (pixils/defmode translucent-panel
      {:style {:width 20 :height 10 :opacity 0.5}
       :render (fn [state ctx]
                 (pixils.render/rect!
                   {:x 0 :y 0}
                   {:x 20 :y 10}
                   {:fill true}))})
  )");
  session.push_mode("translucent-panel", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[0].rendered_rect.w, 20);
  EXPECT_EQ(ops[0].rendered_rect.h, 10);

  ASSERT_EQ(ops[0].sub_ops.size(), 1u);
  EXPECT_EQ(ops[0].sub_ops[0].type, RenderOpType::FILL_RECT);
}

TEST_F(RenderTest, text_without_explicit_color_uses_original_font_texture)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 8 :h 8}}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/text! "A" {:x 12 :y 18} {:font :font/test-font}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  SDL_Texture* original_texture = render_ctx.asset_registry->get_image("fonts", "atlas");
  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");

  ASSERT_NE(original_texture, nullptr);
  ASSERT_NE(tint_texture, nullptr);
  EXPECT_NE(original_texture, tint_texture);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, original_texture);
}

TEST_F(RenderTest, text_with_explicit_color_uses_tint_mask_texture)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 8 :h 8}}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/text!
                  "A"
                  {:x 12 :y 18}
                  {:font :font/test-font
                   :color {:r 200 :g 40 :b 60}}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  SDL_Texture* original_texture = render_ctx.asset_registry->get_image("fonts", "atlas");
  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");

  ASSERT_NE(original_texture, nullptr);
  ASSERT_NE(tint_texture, nullptr);
  EXPECT_NE(original_texture, tint_texture);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, tint_texture);
}

TEST_F(RenderTest, text_accepts_vector_scale_as_x_y)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/defmode test-mode
      {:render (fn [state ctx]
                 (pixils.render/text!
                   "A"
                   {:x 12 :y 18}
                   {:font :font/test-font
                    :scale [2 1]}))})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 8);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
}

TEST_F(RenderTest, text_accepts_fractional_scale)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/defmode test-mode
      {:render (fn [state ctx]
                 (pixils.render/text!
                   "A"
                   {:x 12 :y 18}
                   {:font :font/test-font
                    :scale 1.5}))})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[0].rendered_rect.w, 6);
  EXPECT_EQ(ops[0].rendered_rect.h, 11);
}

TEST_F(RenderTest, text_marked_style_accepts_vector_scale_as_x_y)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont base-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/deffont marked-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode test-mode
      {:render (fn [state ctx]
                 (pixils.render/text!
                   "A@B@"
                   {:x 12 :y 18}
                   {:font :font/base-font
                    :marked-style {:enabled true
                                   :marker "@"
                                   :font :font/marked-font
                                   :scale [2 1]}}))})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 12);
  EXPECT_EQ(ops[0].rendered_rect.w, 4);
  EXPECT_EQ(ops[0].rendered_rect.h, 7);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.x, 17);
  EXPECT_EQ(ops[1].rendered_rect.w, 8);
  EXPECT_EQ(ops[1].rendered_rect.h, 7);
}

TEST_F(RenderTest, text_bang_respects_explicit_newlines)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode test-mode {
      :render (fn [state ctx]
                (pixils.render/text! "AA
B" {:x 12 :y 18} {:font :font/test-font}))
    })
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.y, 18);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.y, 18);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[2].rendered_rect.y, 25);
}

TEST_F(RenderTest, text_size_uses_explicit_font_line_height)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :line-height 10
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}
                "p" {:x 4 :y 0 :w 4 :h 7}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "A" {:font :font/test-font}))");
  auto w = Roo::Dict::get_property(result, Roo::keyword("w"));
  auto h = Roo::Dict::get_property(result, Roo::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 5);
  EXPECT_EQ(h->num().get_int(), 10);
}

TEST_F(RenderTest, text_size_respects_explicit_newlines)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "AA
B" {:font :font/test-font}))");
  auto w = Roo::Dict::get_property(result, Roo::keyword("w"));
  auto h = Roo::Dict::get_property(result, Roo::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 10);
  EXPECT_EQ(h->num().get_int(), 14);
}

TEST_F(RenderTest, text_size_accepts_vector_scale_as_x_y)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
  )");

  auto result =
    runtime.eval(R"((pixils.render/text-size "AA" {:font :font/test-font :scale [2 1]}))");
  auto w = Roo::Dict::get_property(result, Roo::keyword("w"));
  auto h = Roo::Dict::get_property(result, Roo::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 20);
  EXPECT_EQ(h->num().get_int(), 7);
}

TEST_F(RenderTest, text_size_accepts_fractional_scale)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
  )");

  auto result =
    runtime.eval(R"((pixils.render/text-size "AA" {:font :font/test-font :scale 1.5}))");
  auto w = Roo::Dict::get_property(result, Roo::keyword("w"));
  auto h = Roo::Dict::get_property(result, Roo::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 15);
  EXPECT_EQ(h->num().get_int(), 11);
}

TEST_F(RenderTest, text_size_applies_scale_to_ttf_fonts)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 14
       :line-height 14})
  )");

  auto base = runtime.eval(R"((pixils.render/text-size "AA" {:font :font/test-font}))");
  auto scaled =
    runtime.eval(R"((pixils.render/text-size "AA" {:font :font/test-font :scale 2}))");

  auto base_w = Roo::Dict::get_property(base, Roo::keyword("w"));
  auto base_h = Roo::Dict::get_property(base, Roo::keyword("h"));
  auto scaled_w = Roo::Dict::get_property(scaled, Roo::keyword("w"));
  auto scaled_h = Roo::Dict::get_property(scaled, Roo::keyword("h"));

  ASSERT_TRUE(base_w);
  ASSERT_TRUE(base_h);
  ASSERT_TRUE(scaled_w);
  ASSERT_TRUE(scaled_h);
  EXPECT_EQ(scaled_w->num().get_int(), base_w->num().get_int() * 2);
  EXPECT_EQ(scaled_h->num().get_int(), base_h->num().get_int() * 2);
}

TEST_F(RenderTest, text_size_reflects_ttf_declared_size)
{
  runtime.eval(R"(
    (pixils/deffont small-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})
    (pixils/deffont large-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24})
  )");

  auto small = runtime.eval(R"((pixils.render/text-size "AA" {:font :font/small-font}))");
  auto large = runtime.eval(R"((pixils.render/text-size "AA" {:font :font/large-font}))");

  auto small_w = Roo::Dict::get_property(small, Roo::keyword("w"));
  auto small_h = Roo::Dict::get_property(small, Roo::keyword("h"));
  auto large_w = Roo::Dict::get_property(large, Roo::keyword("w"));
  auto large_h = Roo::Dict::get_property(large, Roo::keyword("h"));

  ASSERT_TRUE(small_w);
  ASSERT_TRUE(small_h);
  ASSERT_TRUE(large_w);
  ASSERT_TRUE(large_h);
  EXPECT_GT(large_w->num().get_int(), small_w->num().get_int());
  EXPECT_GT(large_h->num().get_int(), small_h->num().get_int());
}

TEST_F(RenderTest, text_size_reflects_ttf_declared_size_with_smaller_line_height)
{
  runtime.eval(R"(
    (pixils/deffont small-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10
       :line-height 10})
    (pixils/deffont large-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24
       :line-height 10})
  )");

  auto small = runtime.eval(R"((pixils.render/text-size "AA" {:font :font/small-font}))");
  auto large = runtime.eval(R"((pixils.render/text-size "AA" {:font :font/large-font}))");

  auto small_w = Roo::Dict::get_property(small, Roo::keyword("w"));
  auto small_h = Roo::Dict::get_property(small, Roo::keyword("h"));
  auto large_w = Roo::Dict::get_property(large, Roo::keyword("w"));
  auto large_h = Roo::Dict::get_property(large, Roo::keyword("h"));

  ASSERT_TRUE(small_w);
  ASSERT_TRUE(small_h);
  ASSERT_TRUE(large_w);
  ASSERT_TRUE(large_h);
  EXPECT_GT(large_w->num().get_int(), small_w->num().get_int());
  EXPECT_GT(large_h->num().get_int(), small_h->num().get_int());
}

TEST_F(RenderTest, deffont_replaces_existing_font_definition)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 7 :h 4}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "A" {:font :font/test-font}))");
  auto w = Roo::Dict::get_property(result, Roo::keyword("w"));

  ASSERT_TRUE(w);
  EXPECT_EQ(w->num().get_int(), 8);
}

TEST_F(RenderTest, text_size_ignores_inline_toggle_markers)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 3 :h 7}}})
  )");

  auto result = runtime.eval("(pixils.render/text-size \"A@A@\" {:font :font/test-font "
                             ":marked-style {:enabled true :marker \"@\"}})");

  auto w = Roo::Dict::get_property(result, Roo::keyword("w"));
  auto h = Roo::Dict::get_property(result, Roo::keyword("h"));
  ASSERT_NE(w, nullptr);
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(w->num().get_int(), 8);
  EXPECT_EQ(h->num().get_int(), 7);
}

TEST_F(RenderTest, text_cursor_treats_at_markers_as_console_style_toggles)
{
  Pixils::Text::FontMap font_map(
    {{'A', SDL_Rect{0, 0, 4, 7}}, {'B', SDL_Rect{4, 0, 4, 7}}, {'@', SDL_Rect{8, 0, 4, 7}}});
  Pixils::Text::Renderer renderer(render_ctx.buffer_texture, font_map, 1, 1);
  Pixils::Text::Cursor cursor(renderer,
                              SDL_Color{0xff, 0xff, 0xff, 0xff},
                              SDL_Color{0x2b, 0x83, 0x14, 0xff});

  cursor.print(render_ctx, "@A@B");

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[1].rendered_rect.x, 5);

  auto rect = cursor.get_rendered_rect(render_ctx, "@A@B");
  EXPECT_EQ(rect.w, 10);
}

TEST_F(RenderTest, text_cursor_escapes_double_at_as_literal_at_glyph)
{
  Pixils::Text::FontMap font_map({{'@', SDL_Rect{0, 0, 4, 7}}});
  Pixils::Text::Renderer renderer(render_ctx.buffer_texture, font_map, 1, 1);
  Pixils::Text::Cursor cursor(renderer,
                              SDL_Color{0xff, 0xff, 0xff, 0xff},
                              SDL_Color{0x2b, 0x83, 0x14, 0xff});

  cursor.print(render_ctx, "@@");

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
}

TEST_F(RenderTest, deffont_registers_baseline_and_underline_metrics)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 1 :thickness 2}}
       :glyphs {"A" {:x 0 :y 0 :w 3 :h 7}}})
  )");

  auto* font = render_ctx.font_registry->get_font("font/test-font");
  ASSERT_NE(font, nullptr);
  EXPECT_EQ(font->definition.baseline, 5);
  ASSERT_TRUE(font->definition.underline.has_value());
  EXPECT_EQ(font->definition.underline->offset, 1);
  EXPECT_EQ(font->definition.underline->thickness, 2);
}

TEST_F(RenderTest, defbundle_declares_font_resources_for_ttf_fonts)
{
  runtime.eval(R"(
    (pixils/defbundle fonts {:fonts {:autoega "assets/Ac437_STB_AutoEGA_8x14.ttf"}})
  )");

  auto font_path = render_ctx.asset_registry->get_font_path("fonts", "autoega");

  ASSERT_TRUE(font_path.has_value());
  EXPECT_EQ(*font_path, "./assets/Ac437_STB_AutoEGA_8x14.ttf");
}

TEST_F(RenderTest, deffont_ttf_reports_load_failure)
{
  runtime.eval(R"(
    (pixils/defbundle fonts {:fonts {:missing "assets/missing.ttf"}})
  )");

  EXPECT_THROW(runtime.eval(R"(
    (pixils/deffont missing-font
      {:type :ttf
       :resource :fonts/missing
       :size 14})
  )"),
               Roo::InvocationException);
}

TEST_F(RenderTest, text_with_underline_font_style_renders_fill_rect_for_underline)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 1 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 3 :h 7}}})
    (pixils/defmode test-mode
      {:render (fn [state ctx]
                 (pixils.render/text! "A"
                                      {:x 10 :y 12}
                                      {:font :font/test-font
                                       :color {:r 255 :g 255 :b 255}
                                       :font-styles :underline}))})
  )");
  session.push_mode("test-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[1].rendered_rect.x, 10);
  EXPECT_EQ(ops[1].rendered_rect.y, 18);
  EXPECT_EQ(ops[1].rendered_rect.w, 4);
  EXPECT_EQ(ops[1].rendered_rect.h, 1);
}

TEST_F(RenderTest, text_size_infers_font_line_height_from_tallest_glyph)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}
                "p" {:x 4 :y 0 :w 4 :h 7}}})
  )");

  auto result = runtime.eval(R"((pixils.render/text-size "A" {:font :font/test-font}))");
  auto w = Roo::Dict::get_property(result, Roo::keyword("w"));
  auto h = Roo::Dict::get_property(result, Roo::keyword("h"));

  ASSERT_TRUE(w);
  ASSERT_TRUE(h);
  EXPECT_EQ(w->num().get_int(), 5);
  EXPECT_EQ(h->num().get_int(), 7);
}

TEST_F(RenderTest, built_in_text_node_renders_and_measures_without_definition)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}
                "p" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "A"}
                   :style {:text {:font :font/test-font}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);

  auto& child = session.active_mode->children.at(0);
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(child->mode->name, "ui/text");

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 5);
  EXPECT_EQ(child->bounds.h, 7);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
}

TEST_F(RenderTest, built_in_text_node_accepts_fractional_text_scale)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 4}
                "p" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "A"}
                   :style {:text {:font :font/test-font
                                  :scale 1.5}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  auto& child = session.active_mode->children.at(0);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 8);
  EXPECT_EQ(child->bounds.h, 11);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.w, 6);
  EXPECT_EQ(ops[0].rendered_rect.h, 6);
}

TEST_F(RenderTest, built_in_text_node_applies_text_scale_to_ttf_font)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 14
       :line-height 14})
    (pixils/defmode base-mode
      {:children [{:mode 'ui/text
                   :state {:value "AA"}
                   :style {:text {:font :font/test-font}}}]})
    (pixils/defmode scaled-mode
      {:children [{:mode 'ui/text
                   :state {:value "AA"}
                   :style {:text {:font :font/test-font
                                  :scale 2}}}]})
  )");

  session.push_mode("base-mode", Roo::Constant::NIL);
  session.render_mode();
  auto base = session.active_mode->children.at(0)->bounds;

  session.push_mode("scaled-mode", Roo::Constant::NIL);
  session.render_mode();
  auto scaled = session.active_mode->children.at(0)->bounds;

  EXPECT_EQ(scaled.w, base.w * 2);
  EXPECT_EQ(scaled.h, base.h * 2);
}

TEST_F(RenderTest, built_in_text_node_reflects_ttf_declared_size)
{
  runtime.eval(R"(
    (pixils/deffont small-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})
    (pixils/deffont large-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24})
    (pixils/defmode small-mode
      {:children [{:mode 'ui/text
                   :state {:value "AA"}
                   :style {:text {:font :font/small-font}}}]})
    (pixils/defmode large-mode
      {:children [{:mode 'ui/text
                   :state {:value "AA"}
                   :style {:text {:font :font/large-font}}}]})
  )");

  session.push_mode("small-mode", Roo::Constant::NIL);
  session.render_mode();
  auto small = session.active_mode->children.at(0)->bounds;

  session.push_mode("large-mode", Roo::Constant::NIL);
  session.render_mode();
  auto large = session.active_mode->children.at(0)->bounds;

  EXPECT_GT(large.w, small.w);
  EXPECT_GT(large.h, small.h);
}

TEST_F(RenderTest, built_in_text_node_remeasures_when_ttf_font_is_redeclared)
{
  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 10})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "AA"}
                   :style {:text {:font :font/test-font}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  session.render_mode();
  auto small = session.active_mode->children.at(0)->bounds;

  runtime.eval(R"(
    (pixils/deffont test-font
      {:type :ttf
       :resource :pixils/autoega-8x14
       :size 24})
  )");
  session.render_mode();
  auto large = session.active_mode->children.at(0)->bounds;

  EXPECT_GT(large.w, small.w);
  EXPECT_GT(large.h, small.h);
}

TEST_F(RenderTest, built_in_text_node_falls_back_to_console_font_when_style_font_is_missing)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {8, 8};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont console
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 8 :h 8}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "A"}
                   :style {:text {:font :font/missing}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
}

TEST_F(RenderTest, built_in_text_node_renders_in_local_viewport_coordinates)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent padded-box
      {:style {:padding [5 7]}
       :children [{:mode 'ui/text
                   :state {:value "A"}
                   :style {:text {:font :font/test-font}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'padded-box}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 1u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.x, 0);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
}

TEST_F(RenderTest, built_in_text_node_respects_explicit_newlines)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defmode root-mode
      {:children [{:mode 'ui/text
                   :state {:value "AA
B"}
                   :style {:text {:font :font/test-font}}}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);

  auto& child = session.active_mode->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 10);
  EXPECT_EQ(child->bounds.h, 14);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_rect.y, 0);
  EXPECT_EQ(ops[1].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].rendered_rect.y, 0);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[2].rendered_rect.y, 7);
}

TEST_F(RenderTest, built_in_text_node_inherits_marked_style_and_renders_underline)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 2 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent menu-like-item
      {:style {:text {:font :font/test-font
                      :color {:r 0 :g 0 :b 0}
                      :marked-style {:enabled true
                                     :marker "@"
                                     :font-styles :underline}}}
       :children [{:mode 'ui/text
                   :state {:value "@A@"}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'menu-like-item}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);

  ASSERT_NO_THROW(session.render_mode());

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 2u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
}

TEST_F(RenderTest,
       built_in_text_node_marked_style_explicit_color_uses_tint_texture_for_mnemonic)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 2 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent menu-like-item
      {:style {:text {:font :font/test-font
                      :color {:r 0 :g 0 :b 0}
                      :marked-style {:enabled true
                                     :marker "@"
                                     :font-styles :underline
                                     :color {:r 255 :g 255 :b 255}}}}
       :children [{:mode 'ui/text
                   :state {:value "@A@B"}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'menu-like-item}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NO_THROW(session.render_mode());

  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");
  ASSERT_NE(tint_texture, nullptr);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, tint_texture);
  EXPECT_EQ(ops[0].texture_color_mod.r, 255);
  EXPECT_EQ(ops[0].texture_color_mod.g, 255);
  EXPECT_EQ(ops[0].texture_color_mod.b, 255);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[2].texture_color_mod.r, 0);
  EXPECT_EQ(ops[2].texture_color_mod.g, 0);
  EXPECT_EQ(ops[2].texture_color_mod.b, 0);
}

TEST_F(RenderTest, built_in_text_node_marked_style_inherits_parent_tint_for_mnemonic)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :baseline 5
       :styles {:underline {:offset 2 :thickness 1}}
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                "B" {:x 4 :y 0 :w 4 :h 7}}})
    (pixils/defcomponent menu-like-item
      {:style {:text {:font :font/test-font
                      :color {:r 255 :g 255 :b 255}
                      :marked-style {:enabled true
                                     :marker "@"
                                     :font-styles :underline}}}
       :children [{:mode 'ui/text
                   :state {:value "@A@B"}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'menu-like-item}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NO_THROW(session.render_mode());

  SDL_Texture* tint_texture = render_ctx.asset_registry->get_tint_mask("fonts", "atlas");
  ASSERT_NE(tint_texture, nullptr);

  auto& ops = render_target()->render_ops;
  ASSERT_EQ(ops.size(), 3u);
  EXPECT_EQ(ops[0].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[0].rendered_texture, tint_texture);
  EXPECT_EQ(ops[0].texture_color_mod.r, 255);
  EXPECT_EQ(ops[0].texture_color_mod.g, 255);
  EXPECT_EQ(ops[0].texture_color_mod.b, 255);
  EXPECT_EQ(ops[1].type, RenderOpType::FILL_RECT);
  EXPECT_EQ(ops[2].type, RenderOpType::RENDER_COPY);
  EXPECT_EQ(ops[2].texture_color_mod.r, 255);
  EXPECT_EQ(ops[2].texture_color_mod.g, 255);
  EXPECT_EQ(ops[2].texture_color_mod.b, 255);
}

TEST_F(RenderTest, built_in_text_node_wraps_wordwise_when_fill_width_is_constrained)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                " " {:x 4 :y 0 :w 1 :h 7}}})
    (pixils/defcomponent wrap-box
      {:style {:width 12
               :max-width 12}
       :children [{:mode 'ui/text
                   :state {:value "AA AA AA"}
                   :style {:width :fill
                           :text {:font :font/test-font}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'wrap-box}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  auto& child = session.active_mode->children.at(0)->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 12);
  EXPECT_EQ(child->bounds.h, 21);
}

TEST_F(RenderTest, built_in_text_node_wraps_wordwise_when_parent_width_is_constrained)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                " " {:x 4 :y 0 :w 1 :h 7}}})
    (pixils/defcomponent wrap-box
      {:style {:width 12
               :max-width 12}
       :children [{:mode 'ui/text
                   :state {:value "AA AA AA"}
                   :style {:text {:font :font/test-font}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'wrap-box}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  auto& child = session.active_mode->children.at(0)->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 10);
  EXPECT_EQ(child->bounds.h, 21);
}

TEST_F(RenderTest, built_in_text_node_wrap_none_stays_single_line_when_width_is_constrained)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {16, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"A" {:x 0 :y 0 :w 4 :h 7}
                " " {:x 4 :y 0 :w 1 :h 7}}})
    (pixils/defcomponent wrap-box
      {:style {:width 12
               :max-width 12}
       :children [{:mode 'ui/text
                   :state {:value "AA AA AA"}
                   :style {:width :fill
                           :text {:font :font/test-font
                                  :wrap :none}}}]})
    (pixils/defmode root-mode
      {:children [{:mode 'wrap-box}]})
  )");

  session.push_mode("root-mode", Roo::Constant::NIL);
  ASSERT_NE(session.active_mode, nullptr);
  ASSERT_EQ(session.active_mode->children.size(), 1u);
  ASSERT_EQ(session.active_mode->children.at(0)->children.size(), 1u);

  auto& child = session.active_mode->children.at(0)->children.at(0);
  ASSERT_NE(child, nullptr);

  ASSERT_NO_THROW(session.render_mode());

  EXPECT_EQ(child->bounds.w, 12);
  EXPECT_EQ(child->bounds.h, 7);
}

TEST_F(RenderTest, built_in_text_node_wrap_preserves_leading_spaces)
{
  SDL3Mock::prepared_surfaces["./font.png"] = {24, 12};
  runtime.eval(R"(
    (pixils/defbundle fonts {:images {:atlas "font.png"}})
    (pixils/deffont test-font
      {:type :bitmap
       :resource :fonts/atlas
       :glyphs {"O" {:x 0 :y 0 :w 4 :h 7}
                "K" {:x 4 :y 0 :w 4 :h 7}
                " " {:x 8 :y 0 :w 1 :h 7}}})
  )");

  auto text_op = Pixils::Text::make_text_render_op(render_ctx, "font/test-font", 1);
  ASSERT_TRUE(text_op.has_value());

  auto layout = Pixils::Text::layout_text(render_ctx,
                                          *text_op,
                                          "  OK  ",
                                          Pixils::Text::WrapMode::WORD,
                                          24);
  ASSERT_EQ(layout.lines.size(), 1u);
  EXPECT_EQ(layout.lines[0].text, "  OK  ");
  EXPECT_EQ(layout.size.w, 18);
  EXPECT_EQ(layout.size.h, 7);
}
