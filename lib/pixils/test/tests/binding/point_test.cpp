#include "../fixture.h"

#include <gtest/gtest.h>

class PointTest : public BaseFixture
{
};

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

TEST_F(PointTest, clamp_limits_point_to_rect_bounds)
{
  auto x = runtime.eval("(:x (pixils.point/clamp {:x -5 :y 30} {:x 10 :y 20 :w 100 :h 50}))");
  auto y = runtime.eval("(:y (pixils.point/clamp {:x -5 :y 30} {:x 10 :y 20 :w 100 :h 50}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 10);
  EXPECT_EQ(y->num().get_int(), 30);
}

TEST_F(PointTest, wrap_repositions_point_to_opposite_edge_when_outside_bounds)
{
  auto left_x = runtime.eval("(:x (pixils.point/wrap {:x 9 :y 30} {:x 10 :y 20 :w 100 :h 50}))");
  auto right_x = runtime.eval("(:x (pixils.point/wrap {:x 111 :y 30} {:x 10 :y 20 :w 100 :h 50}))");
  auto top_y = runtime.eval("(:y (pixils.point/wrap {:x 20 :y 19} {:x 10 :y 20 :w 100 :h 50}))");
  auto bottom_y = runtime.eval("(:y (pixils.point/wrap {:x 20 :y 71} {:x 10 :y 20 :w 100 :h 50}))");

  ASSERT_NE(left_x, nullptr);
  ASSERT_NE(right_x, nullptr);
  ASSERT_NE(top_y, nullptr);
  ASSERT_NE(bottom_y, nullptr);
  EXPECT_EQ(left_x->num().get_int(), 110);
  EXPECT_EQ(right_x->num().get_int(), 10);
  EXPECT_EQ(top_y->num().get_int(), 70);
  EXPECT_EQ(bottom_y->num().get_int(), 20);
}
