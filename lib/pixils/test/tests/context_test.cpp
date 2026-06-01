#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/script.h>

#include <gtest/gtest.h>

namespace
{
  Pixils::Display fixed_display(
    Pixils::Display::Scaling scaling,
    Pixils::Display::Alignment align = Pixils::Display::Alignment::CENTER)
  {
    return Pixils::Display(
      Pixils::Resolution(Pixils::Resolution::Mode::FIXED, Pixils::Dimension{640, 360}),
      align,
      scaling,
      Pixils::Color{0, 0, 0});
  }
} // namespace

TEST(RenderContextCoordinateMappingTest, fit_centered_maps_window_point_through_letterbox)
{
  Pixils::RenderContext ctx;
  ctx.window_rect = {0, 0, 800, 600};
  ctx.buffer_dim = {640, 360};
  auto display = fixed_display(Pixils::Display::Scaling::FIT);

  EXPECT_EQ(ctx.application_target_rect(display), (Pixils::Rect{80, 120, 640, 360}));
  ctx.application_rect = ctx.application_target_rect(display);

  EXPECT_EQ(ctx.window_to_buffer_point(display, 80, 120), (Pixils::Point{0, 0}));
  EXPECT_EQ(ctx.window_to_buffer_point(display, 400, 300), (Pixils::Point{320, 180}));
  EXPECT_EQ(ctx.buffer_to_window_point({320, 180}), (Pixils::Point{400, 300}));
}

TEST(RenderContextCoordinateMappingTest, fit_centered_maps_scaled_window_point)
{
  Pixils::RenderContext ctx;
  ctx.window_rect = {0, 0, 1280, 720};
  ctx.buffer_dim = {640, 360};
  auto display = fixed_display(Pixils::Display::Scaling::FIT);

  EXPECT_EQ(ctx.application_target_rect(display), (Pixils::Rect{0, 0, 1280, 720}));
  ctx.application_rect = ctx.application_target_rect(display);
  EXPECT_EQ(ctx.window_to_buffer_point(display, 640, 360), (Pixils::Point{320, 180}));
  EXPECT_EQ(ctx.buffer_to_window_point({320, 180}), (Pixils::Point{640, 360}));
}

TEST(RenderContextCoordinateMappingTest, stretch_maps_proportionally)
{
  Pixils::RenderContext ctx;
  ctx.window_rect = {0, 0, 800, 600};
  ctx.buffer_dim = {640, 360};
  auto display = fixed_display(Pixils::Display::Scaling::STRETCH);

  EXPECT_EQ(ctx.application_target_rect(display), (Pixils::Rect{0, 0, 800, 600}));
  ctx.application_rect = ctx.application_target_rect(display);
  EXPECT_EQ(ctx.window_to_buffer_point(display, 400, 300), (Pixils::Point{320, 180}));
  EXPECT_EQ(ctx.buffer_to_window_point({320, 180}), (Pixils::Point{400, 300}));
}

TEST(RenderContextMouseWarpTest, global_warp_mouse_function_returns_requested_point)
{
  Pixils::RenderContext ctx;
  Roo::Runtime runtime = Pixils::init_roo_runtime(ctx, "test", {});

  auto x = runtime.eval("(:x (pixils/warp-mouse! {:x 12 :y 34}))");
  auto y = runtime.eval("(:y (pixils/warp-mouse! {:x 12 :y 34}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 12);
  EXPECT_EQ(y->num().get_int(), 34);
}
