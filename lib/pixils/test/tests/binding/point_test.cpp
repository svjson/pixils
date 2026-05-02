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
