#include <pixils/context.h>
#include <pixils/display.h>

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

  EXPECT_EQ(ctx.window_to_buffer_point(display, 80, 120), (Pixils::Point{0, 0}));
  EXPECT_EQ(ctx.window_to_buffer_point(display, 400, 300), (Pixils::Point{320, 180}));
}

TEST(RenderContextCoordinateMappingTest, fit_centered_maps_scaled_window_point)
{
  Pixils::RenderContext ctx;
  ctx.window_rect = {0, 0, 1280, 720};
  ctx.buffer_dim = {640, 360};
  auto display = fixed_display(Pixils::Display::Scaling::FIT);

  EXPECT_EQ(ctx.application_target_rect(display), (Pixils::Rect{0, 0, 1280, 720}));
  EXPECT_EQ(ctx.window_to_buffer_point(display, 640, 360), (Pixils::Point{320, 180}));
}

TEST(RenderContextCoordinateMappingTest, stretch_maps_proportionally)
{
  Pixils::RenderContext ctx;
  ctx.window_rect = {0, 0, 800, 600};
  ctx.buffer_dim = {640, 360};
  auto display = fixed_display(Pixils::Display::Scaling::STRETCH);

  EXPECT_EQ(ctx.application_target_rect(display), (Pixils::Rect{0, 0, 800, 600}));
  EXPECT_EQ(ctx.window_to_buffer_point(display, 400, 300), (Pixils::Point{320, 180}));
}
