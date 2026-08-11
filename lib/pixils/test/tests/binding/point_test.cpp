#include "../fixture.h"

#include <gtest/gtest.h>

class PointTest : public BaseFixture
{
};

TEST_F(PointTest, equality_compares_both_axes)
{
  EXPECT_EQ(runtime.eval("(pixils.point/= {:x 10 :y 20} {:x 10 :y 20})"),
            Roo::Constant::BOOL_TRUE);
  EXPECT_EQ(runtime.eval("(pixils.point/= {:x 10 :y 20} {:x 11 :y 20})"),
            Roo::Constant::BOOL_FALSE);
  EXPECT_EQ(runtime.eval("(pixils.point/= {:x 10 :y 20} {:x 10 :y 21})"),
            Roo::Constant::BOOL_FALSE);
}

TEST_F(PointTest, translate_moves_point_by_dx_and_dy)
{
  auto x = runtime.eval("(:x (pixils.point/translate {:x 10 :y 20} 3 -4))");
  auto y = runtime.eval("(:y (pixils.point/translate {:x 10 :y 20} 3 -4))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 13);
  EXPECT_EQ(y->num().get_int(), 16);
}

TEST_F(PointTest, translate_x_moves_only_x_axis)
{
  auto x = runtime.eval("(:x (pixils.point/translate-x {:x 10 :y 20} -5))");
  auto y = runtime.eval("(:y (pixils.point/translate-x {:x 10 :y 20} -5))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 5);
  EXPECT_EQ(y->num().get_int(), 20);
}

TEST_F(PointTest, translate_y_moves_only_y_axis)
{
  auto x = runtime.eval("(:x (pixils.point/translate-y {:x 10 :y 20} 7))");
  auto y = runtime.eval("(:y (pixils.point/translate-y {:x 10 :y 20} 7))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 10);
  EXPECT_EQ(y->num().get_int(), 27);
}

TEST_F(PointTest, min_limits_each_axis_to_scalar_bound)
{
  auto x = runtime.eval("(:x (pixils.point/min {:x 10 :y 20} 15))");
  auto y = runtime.eval("(:y (pixils.point/min {:x 10 :y 20} 15))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 10);
  EXPECT_EQ(y->num().get_int(), 15);
}

TEST_F(PointTest, min_limits_each_axis_to_point_bound)
{
  auto x = runtime.eval("(:x (pixils.point/min {:x 10 :y 20} {:x 8 :y 30}))");
  auto y = runtime.eval("(:y (pixils.point/min {:x 10 :y 20} {:x 8 :y 30}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 8);
  EXPECT_EQ(y->num().get_int(), 20);
}

TEST_F(PointTest, max_limits_each_axis_to_scalar_bound)
{
  auto x = runtime.eval("(:x (pixils.point/max {:x -5 :y 20} 0))");
  auto y = runtime.eval("(:y (pixils.point/max {:x -5 :y 20} 0))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 0);
  EXPECT_EQ(y->num().get_int(), 20);
}

TEST_F(PointTest, max_limits_each_axis_to_point_bound)
{
  auto x = runtime.eval("(:x (pixils.point/max {:x -5 :y 20} {:x -2 :y 30}))");
  auto y = runtime.eval("(:y (pixils.point/max {:x -5 :y 20} {:x -2 :y 30}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), -2);
  EXPECT_EQ(y->num().get_int(), 30);
}

TEST_F(PointTest, clamp_limits_point_to_rect_bounds)
{
  auto x =
    runtime.eval("(:x (pixils.point/clamp {:x -5 :y 30} {:x 10 :y 20 :w 100 :h 50}))");
  auto y =
    runtime.eval("(:y (pixils.point/clamp {:x -5 :y 30} {:x 10 :y 20 :w 100 :h 50}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 10);
  EXPECT_EQ(y->num().get_int(), 30);
}

TEST_F(PointTest, clamp_limits_point_to_scalar_bounds)
{
  auto x = runtime.eval("(:x (pixils.point/clamp {:x -5 :y 20} 0 15))");
  auto y = runtime.eval("(:y (pixils.point/clamp {:x -5 :y 20} 0 15))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 0);
  EXPECT_EQ(y->num().get_int(), 15);
}

TEST_F(PointTest, clamp_limits_point_to_asymmetric_point_bounds)
{
  auto x = runtime.eval("(:x (pixils.point/clamp {:x -20 :y 80}"
                        "                         {:x -15 :y -30}"
                        "                         {:x 15 :y 55}))");
  auto y = runtime.eval("(:y (pixils.point/clamp {:x -20 :y 80}"
                        "                         {:x -15 :y -30}"
                        "                         {:x 15 :y 55}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), -15);
  EXPECT_EQ(y->num().get_int(), 55);
}

TEST_F(PointTest, wrap_repositions_point_to_opposite_edge_when_outside_bounds)
{
  auto left_x =
    runtime.eval("(:x (pixils.point/wrap {:x 9 :y 30} {:x 10 :y 20 :w 100 :h 50}))");
  auto right_x =
    runtime.eval("(:x (pixils.point/wrap {:x 111 :y 30} {:x 10 :y 20 :w 100 :h 50}))");
  auto top_y =
    runtime.eval("(:y (pixils.point/wrap {:x 20 :y 19} {:x 10 :y 20 :w 100 :h 50}))");
  auto bottom_y =
    runtime.eval("(:y (pixils.point/wrap {:x 20 :y 71} {:x 10 :y 20 :w 100 :h 50}))");

  ASSERT_NE(left_x, nullptr);
  ASSERT_NE(right_x, nullptr);
  ASSERT_NE(top_y, nullptr);
  ASSERT_NE(bottom_y, nullptr);
  EXPECT_EQ(left_x->num().get_int(), 110);
  EXPECT_EQ(right_x->num().get_int(), 10);
  EXPECT_EQ(top_y->num().get_int(), 70);
  EXPECT_EQ(bottom_y->num().get_int(), 20);
}

TEST_F(PointTest, distance_squared_returns_squared_euclidean_distance)
{
  auto distance = runtime.eval("(pixils.point/distance-squared {:x 0 :y 0} {:x 3 :y 4})");

  ASSERT_NE(distance, nullptr);
  EXPECT_FLOAT_EQ(distance->f32(), 25.0f);
}
