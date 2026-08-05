#include "../fixture.h"

#include <gtest/gtest.h>

class RectTest : public BaseFixture
{
};

TEST_F(RectTest, normalize_point_returns_normalized_position_inside_rect)
{
  auto x = runtime.eval("(:x (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 0.25 0.5))");
  auto y = runtime.eval("(:y (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 0.25 0.5))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 12.0f);
  EXPECT_FLOAT_EQ(y->f32(), 23.0f);
}

TEST_F(RectTest, normalize_point_does_not_clamp_normalized_factors)
{
  auto x = runtime.eval("(:x (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 1.25 -0.5))");
  auto y = runtime.eval("(:y (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 1.25 -0.5))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 20.0f);
  EXPECT_FLOAT_EQ(y->f32(), 17.0f);
}
