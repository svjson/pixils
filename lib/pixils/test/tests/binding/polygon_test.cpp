#include "../fixture.h"

#include <gtest/gtest.h>
#include <string>

class PolygonTest : public BaseFixture
{
};

namespace
{
  bool eval_bool(Roo::Runtime& runtime, const std::string& source)
  {
    auto result = runtime.eval(source);
    return result && Roo::is_truthy(*result);
  }
} // namespace

TEST_F(PolygonTest, polygon_bounds_returns_integer_rect_covering_points)
{
  auto x = runtime.eval("(:x (pixils.polygon/bounds [{:x 1.2 :y 3.9} {:x 5.1 :y 8.0}]))");
  auto y = runtime.eval("(:y (pixils.polygon/bounds [{:x 1.2 :y 3.9} {:x 5.1 :y 8.0}]))");
  auto w = runtime.eval("(:w (pixils.polygon/bounds [{:x 1.2 :y 3.9} {:x 5.1 :y 8.0}]))");
  auto h = runtime.eval("(:h (pixils.polygon/bounds [{:x 1.2 :y 3.9} {:x 5.1 :y 8.0}]))");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  ASSERT_NE(w, nullptr);
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(x->num().get_int(), 1);
  EXPECT_EQ(y->num().get_int(), 3);
  EXPECT_EQ(w->num().get_int(), 5);
  EXPECT_EQ(h->num().get_int(), 5);
}

TEST_F(PolygonTest, polygon_bounds_returns_nil_for_empty_polygon)
{
  auto result = runtime.eval("(pixils.polygon/bounds [])");
  EXPECT_EQ(result, Roo::Constant::NIL);
}

TEST_F(PolygonTest, polygon_area_returns_absolute_shoelace_area)
{
  auto clockwise = runtime.eval(
    "(pixils.polygon/area [{:x 0 :y 0} {:x 0 :y 10} {:x 10 :y 10} {:x 10 :y 0}])");
  auto counter_clockwise = runtime.eval(
    "(pixils.polygon/area [{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}])");

  ASSERT_NE(clockwise, nullptr);
  ASSERT_NE(counter_clockwise, nullptr);
  EXPECT_FLOAT_EQ(clockwise->f32(), 100.0f);
  EXPECT_FLOAT_EQ(counter_clockwise->f32(), 100.0f);
}

TEST_F(PolygonTest, polygon_vertex_center_returns_average_of_vertices)
{
  auto x = runtime.eval("(:x (pixils.polygon/vertex-center "
                        "[{:x 0 :y 0} {:x 6 :y 0} {:x 0 :y 6}]))");
  auto y = runtime.eval("(:y (pixils.polygon/vertex-center "
                        "[{:x 0 :y 0} {:x 6 :y 0} {:x 0 :y 6}]))");
  auto empty = runtime.eval("(pixils.polygon/vertex-center [])");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 2.0f);
  EXPECT_FLOAT_EQ(y->f32(), 2.0f);
  EXPECT_EQ(empty, Roo::Constant::NIL);
}

TEST_F(PolygonTest, polygon_closest_edge_point_returns_closest_point_on_boundary)
{
  auto x = runtime.eval("(:x (pixils.polygon/closest-edge-point "
                        "[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                        "{:x 4 :y 12}))");
  auto y = runtime.eval("(:y (pixils.polygon/closest-edge-point "
                        "[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                        "{:x 4 :y 12}))");
  auto empty = runtime.eval("(pixils.polygon/closest-edge-point [] {:x 4 :y 12})");

  ASSERT_NE(x, nullptr);
  ASSERT_NE(y, nullptr);
  EXPECT_FLOAT_EQ(x->f32(), 4.0f);
  EXPECT_FLOAT_EQ(y->f32(), 10.0f);
  EXPECT_EQ(empty, Roo::Constant::NIL);
}

TEST_F(PolygonTest, polygon_contains_point_including_boundary)
{
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.polygon/contains? "
                        "[{:x 0 :y 0} {:x 10 :y 0} {:x 0 :y 10}] "
                        "{:x 2 :y 2})"));
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.polygon/contains? "
                        "[{:x 0 :y 0} {:x 10 :y 0} {:x 0 :y 10}] "
                        "{:x 5 :y 0})"));
  EXPECT_FALSE(eval_bool(runtime,
                         "(pixils.polygon/contains? "
                         "[{:x 0 :y 0} {:x 10 :y 0} {:x 0 :y 10}] "
                         "{:x 9 :y 9})"));
}

TEST_F(PolygonTest, polygon_contains_rect_and_polygon)
{
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.polygon/contains? "
                        "[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                        "{:x 2 :y 2 :w 3 :h 3})"));
  EXPECT_FALSE(eval_bool(runtime,
                         "(pixils.polygon/contains? "
                         "[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                         "{:x 8 :y 8 :w 4 :h 4})"));
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.polygon/contains? "
                        "[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                        "[{:x 1 :y 1} {:x 3 :y 1} {:x 3 :y 3} {:x 1 :y 3}])"));
}

TEST_F(PolygonTest, polygons_intersect_on_overlap_or_edge_touch)
{
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.polygon/intersects? "
                        "[{:x 0 :y 0} {:x 5 :y 0} {:x 5 :y 5} {:x 0 :y 5}] "
                        "[{:x 4 :y 4} {:x 8 :y 4} {:x 8 :y 8} {:x 4 :y 8}])"));
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.polygon/intersects? "
                        "[{:x 0 :y 0} {:x 5 :y 0} {:x 5 :y 5} {:x 0 :y 5}] "
                        "[{:x 5 :y 0} {:x 8 :y 0} {:x 8 :y 3} {:x 5 :y 3}])"));
  EXPECT_FALSE(eval_bool(runtime,
                         "(pixils.polygon/intersects? "
                         "[{:x 0 :y 0} {:x 5 :y 0} {:x 5 :y 5} {:x 0 :y 5}] "
                         "[{:x 6 :y 6} {:x 8 :y 6} {:x 8 :y 8} {:x 6 :y 8}])"));
}

TEST_F(PolygonTest, rect_contains_point_rect_and_polygon)
{
  EXPECT_TRUE(
    eval_bool(runtime, "(pixils.rect/contains? {:x 0 :y 0 :w 10 :h 10} {:x 3 :y 4})"));
  EXPECT_FALSE(
    eval_bool(runtime, "(pixils.rect/contains? {:x 0 :y 0 :w 10 :h 10} {:x 10 :y 4})"));
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.rect/contains? "
                        "{:x 0 :y 0 :w 10 :h 10} "
                        "{:x 2 :y 2 :w 3 :h 3})"));
  EXPECT_FALSE(eval_bool(runtime,
                         "(pixils.rect/contains? "
                         "{:x 0 :y 0 :w 10 :h 10} "
                         "{:x 8 :y 8 :w 4 :h 4})"));
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.rect/contains? "
                        "{:x 0 :y 0 :w 10 :h 10} "
                        "[{:x 1 :y 1} {:x 3 :y 1} {:x 3 :y 3}])"));
}

TEST_F(PolygonTest, rect_intersects_rect_and_polygon)
{
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.rect/intersects? "
                        "{:x 2 :y 2 :w 3 :h 3} "
                        "{:x 0 :y 0 :w 10 :h 10})"));
  EXPECT_FALSE(eval_bool(runtime,
                         "(pixils.rect/intersects? "
                         "{:x 0 :y 0 :w 2 :h 2} "
                         "{:x 2 :y 0 :w 2 :h 2})"));
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.rect/intersects? "
                        "{:x 0 :y 0 :w 2 :h 2} "
                        "{:x 2 :y 0 :w 2 :h 2} "
                        "{:include-boundary? true})"));
  EXPECT_TRUE(eval_bool(runtime,
                        "(pixils.rect/intersects? "
                        "{:x 4 :y 4 :w 4 :h 4} "
                        "[{:x 0 :y 0} {:x 6 :y 0} {:x 6 :y 6} {:x 0 :y 6}])"));
  EXPECT_FALSE(eval_bool(runtime,
                         "(pixils.rect/intersects? "
                         "{:x 0 :y 0 :w 2 :h 2} "
                         "[{:x 2 :y 0} {:x 4 :y 0} {:x 4 :y 2} {:x 2 :y 2}])"));
}
