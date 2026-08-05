#include "../fixture.h"

#include <gtest/gtest.h>

class RectTest : public BaseFixture
{
};

TEST_F(RectTest, align_returns_rect_for_numeric_factors)
{
  auto x = runtime.eval(
    "(:x (pixils.rect/align {:x 10 :y 20 :w 100 :h 50} {:w 20 :h 10} 0.5 0.5))");
  auto y = runtime.eval(
    "(:y (pixils.rect/align {:x 10 :y 20 :w 100 :h 50} {:w 20 :h 10} 0.5 0.5))");
  auto w = runtime.eval(
    "(:w (pixils.rect/align {:x 10 :y 20 :w 100 :h 50} {:w 20 :h 10} 0.5 0.5))");
  auto h = runtime.eval(
    "(:h (pixils.rect/align {:x 10 :y 20 :w 100 :h 50} {:w 20 :h 10} 0.5 0.5))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_NE(w, nullptr);
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(x->num().get_int(), 50);
  EXPECT_EQ(y->num().get_int(), 40);
  EXPECT_EQ(w->num().get_int(), 20);
  EXPECT_EQ(h->num().get_int(), 10);
}

TEST_F(RectTest, align_accepts_keyword_anchor)
{
  auto x = runtime.eval(
    "(:x (pixils.rect/align {:x 10 :y 20 :w 100 :h 50} {:w 20 :h 10} :bottom-center))");
  auto y = runtime.eval(
    "(:y (pixils.rect/align {:x 10 :y 20 :w 100 :h 50} {:w 20 :h 10} :bottom-center))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 50);
  EXPECT_EQ(y->num().get_int(), 60);
}

TEST_F(RectTest, align_accepts_at_and_origin_options)
{
  auto x = runtime.eval("(:x (pixils.rect/align {:x 10 :y 20 :w 100 :h 50}"
                        "                         {:w 20 :h 10}"
                        "                         {:at :center :origin :top-left}))");
  auto y = runtime.eval("(:y (pixils.rect/align {:x 10 :y 20 :w 100 :h 50}"
                        "                         {:w 20 :h 10}"
                        "                         {:at :center :origin :top-left}))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_EQ(x->num().get_int(), 60);
  EXPECT_EQ(y->num().get_int(), 45);
}

TEST_F(RectTest, normalize_point_returns_normalized_position_inside_rect)
{
  auto x =
    runtime.eval("(:x (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 0.25 0.5))");
  auto y =
    runtime.eval("(:y (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 0.25 0.5))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 12.0f);
  EXPECT_FLOAT_EQ(y->f32(), 23.0f);
}

TEST_F(RectTest, normalize_point_does_not_clamp_normalized_factors)
{
  auto x =
    runtime.eval("(:x (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 1.25 -0.5))");
  auto y =
    runtime.eval("(:y (pixils.rect/normalize-point {:x 10 :y 20 :w 8 :h 6} 1.25 -0.5))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 20.0f);
  EXPECT_FLOAT_EQ(y->f32(), 17.0f);
}
