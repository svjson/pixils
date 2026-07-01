#include "../fixture.h"

#include <gtest/gtest.h>

class LineTest : public BaseFixture
{
};

TEST_F(LineTest, closest_point_projects_point_onto_finite_line)
{
  auto x = runtime.eval("(:x (pixils.line/closest-point "
                        "{:x 0 :y 0} {:x 10 :y 0} {:x 4 :y 3}))");
  auto y = runtime.eval("(:y (pixils.line/closest-point "
                        "{:x 0 :y 0} {:x 10 :y 0} {:x 4 :y 3}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 4.0f);
  EXPECT_FLOAT_EQ(y->f32(), 0.0f);
}

TEST_F(LineTest, closest_point_clamps_to_finite_line_endpoints)
{
  auto x = runtime.eval("(:x (pixils.line/closest-point "
                        "{:x 0 :y 0} {:x 10 :y 0} {:x 12 :y 3}))");
  auto y = runtime.eval("(:y (pixils.line/closest-point "
                        "{:x 0 :y 0} {:x 10 :y 0} {:x 12 :y 3}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 10.0f);
  EXPECT_FLOAT_EQ(y->f32(), 0.0f);
}
