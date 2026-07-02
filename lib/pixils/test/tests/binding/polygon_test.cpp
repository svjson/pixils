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

TEST_F(PolygonTest, polygon_circle_generates_default_point_count)
{
  auto count = runtime.eval("(count (pixils.polygon/circle {:x 10 :y 20 :r 4}))");
  auto first_x = runtime.eval("(:x (nth (pixils.polygon/circle {:x 10 :y 20 :r 4}) 0))");
  auto first_y = runtime.eval("(:y (nth (pixils.polygon/circle {:x 10 :y 20 :r 4}) 0))");

  ASSERT_NE(count, nullptr);
  ASSERT_NE(first_x, nullptr);
  ASSERT_NE(first_y, nullptr);
  EXPECT_EQ(count->num().get_int(), 32);
  EXPECT_FLOAT_EQ(first_x->f32(), 14.0f);
  EXPECT_FLOAT_EQ(first_y->f32(), 20.0f);
}

TEST_F(PolygonTest, polygon_circle_accepts_segments_and_rotation)
{
  auto count = runtime.eval("(count (pixils.polygon/circle {:x 10 :y 20 :r 4} {:segments 4 "
                            ":rotation 1.57079632679}))");
  auto first_x = runtime.eval("(:x (nth (pixils.polygon/circle {:x 10 :y 20 :r 4} "
                              "{:segments 4 :rotation 1.57079632679}) 0))");
  auto first_y = runtime.eval("(:y (nth (pixils.polygon/circle {:x 10 :y 20 :r 4} "
                              "{:segments 4 :rotation 1.57079632679}) 0))");

  ASSERT_NE(count, nullptr);
  ASSERT_NE(first_x, nullptr);
  ASSERT_NE(first_y, nullptr);
  EXPECT_EQ(count->num().get_int(), 4);
  EXPECT_NEAR(first_x->f32(), 10.0f, 0.0001f);
  EXPECT_FLOAT_EQ(first_y->f32(), 24.0f);
}

TEST_F(PolygonTest, polygon_ellipse_generates_scaled_points)
{
  auto count =
    runtime.eval("(count (pixils.polygon/ellipse {:x 10 :y 20 :rx 8 :ry 4} {:segments 4}))");
  auto first_x = runtime.eval(
    "(:x (nth (pixils.polygon/ellipse {:x 10 :y 20 :rx 8 :ry 4} {:segments 4}) 0))");
  auto second_y = runtime.eval(
    "(:y (nth (pixils.polygon/ellipse {:x 10 :y 20 :rx 8 :ry 4} {:segments 4}) 1))");

  ASSERT_NE(count, nullptr);
  ASSERT_NE(first_x, nullptr);
  ASSERT_NE(second_y, nullptr);
  EXPECT_EQ(count->num().get_int(), 4);
  EXPECT_FLOAT_EQ(first_x->f32(), 18.0f);
  EXPECT_FLOAT_EQ(second_y->f32(), 24.0f);
}

TEST_F(PolygonTest, polygon_shape_generators_return_empty_for_non_positive_radius)
{
  auto circle_count = runtime.eval("(count (pixils.polygon/circle {:x 10 :y 20 :r 0}))");
  auto ellipse_count =
    runtime.eval("(count (pixils.polygon/ellipse {:x 10 :y 20 :rx -1 :ry 4}))");

  ASSERT_NE(circle_count, nullptr);
  ASSERT_NE(ellipse_count, nullptr);
  EXPECT_EQ(circle_count->num().get_int(), 0);
  EXPECT_EQ(ellipse_count->num().get_int(), 0);
}

TEST_F(PolygonTest, polygon_shape_generators_reject_too_few_segments)
{
  EXPECT_THROW(runtime.eval("(pixils.polygon/circle {:x 10 :y 20 :r 4} {:segments 2})"),
               Roo::TypeError);
  EXPECT_THROW(runtime.eval("(pixils.polygon/ellipse {:x 10 :y 20 :rx 4 :ry 8} "
                            "{:segments 2})"),
               Roo::TypeError);
}

TEST_F(PolygonTest, polygon_intersection_returns_overlapping_regions)
{
  auto count = runtime.eval("(count (pixils.polygon/intersection "
                            "[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                            "[{:x 5 :y 5} {:x 15 :y 5} {:x 15 :y 15} {:x 5 :y 15}]))");
  auto area = runtime.eval("(pixils.polygon/area "
                           "(nth (pixils.polygon/intersection "
                           "[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                           "[{:x 5 :y 5} {:x 15 :y 5} {:x 15 :y 15} {:x 5 :y 15}]) "
                           "0))");

  ASSERT_NE(count, nullptr);
  ASSERT_NE(area, nullptr);
  EXPECT_EQ(count->num().get_int(), 1);
  EXPECT_NEAR(area->f32(), 25.0f, 0.0001f);
}

TEST_F(PolygonTest, polygon_intersection_returns_empty_for_disjoint_polygons)
{
  auto count = runtime.eval("(count (pixils.polygon/intersection "
                            "[{:x 0 :y 0} {:x 4 :y 0} {:x 4 :y 4} {:x 0 :y 4}] "
                            "[{:x 5 :y 5} {:x 9 :y 5} {:x 9 :y 9} {:x 5 :y 9}]))");

  ASSERT_NE(count, nullptr);
  EXPECT_EQ(count->num().get_int(), 0);
}

TEST_F(PolygonTest, polygon_intersection_accepts_polygon_lists)
{
  auto count = runtime.eval("(count (pixils.polygon/intersection "
                            "[[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}]] "
                            "[[{:x 5 :y 5} {:x 15 :y 5} {:x 15 :y 15} {:x 5 :y 15}]]))");

  ASSERT_NE(count, nullptr);
  EXPECT_EQ(count->num().get_int(), 1);
}

TEST_F(PolygonTest, polygon_union_merges_overlapping_polygons)
{
  auto count = runtime.eval("(count (pixils.polygon/union "
                            "[[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}]] "
                            "[[{:x 5 :y 5} {:x 15 :y 5} {:x 15 :y 15} {:x 5 :y 15}]]))");
  auto area = runtime.eval("(pixils.polygon/area "
                           "(nth (pixils.polygon/union "
                           "[[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}]] "
                           "[[{:x 5 :y 5} {:x 15 :y 5} {:x 15 :y 15} {:x 5 :y 15}]]"
                           ") 0))");

  ASSERT_NE(count, nullptr);
  ASSERT_NE(area, nullptr);
  EXPECT_EQ(count->num().get_int(), 1);
  EXPECT_NEAR(area->f32(), 175.0f, 0.0001f);
}

TEST_F(PolygonTest, polygon_union_merges_touching_polygons)
{
  auto count = runtime.eval("(count (pixils.polygon/union "
                            "[[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                            " [{:x 10 :y 0} {:x 20 :y 0} {:x 20 :y 10} {:x 10 :y 10}]]))");
  auto area = runtime.eval("(pixils.polygon/area "
                           "(nth (pixils.polygon/union "
                           "[[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                           " [{:x 10 :y 0} {:x 20 :y 0} {:x 20 :y 10} {:x 10 :y 10}]]"
                           ") 0))");

  ASSERT_NE(count, nullptr);
  ASSERT_NE(area, nullptr);
  EXPECT_EQ(count->num().get_int(), 1);
  EXPECT_NEAR(area->f32(), 200.0f, 0.0001f);
}

TEST_F(PolygonTest, polygon_union_keeps_disconnected_islands)
{
  auto count = runtime.eval("(count (pixils.polygon/union "
                            "[[{:x 0 :y 0} {:x 10 :y 0} {:x 10 :y 10} {:x 0 :y 10}] "
                            " [{:x 20 :y 0} {:x 30 :y 0} {:x 30 :y 10} {:x 20 :y 10}]]))");

  ASSERT_NE(count, nullptr);
  EXPECT_EQ(count->num().get_int(), 2);
}

TEST_F(PolygonTest, polygon_boolean_ops_accept_precision_option)
{
  auto area = runtime.eval("(pixils.polygon/area "
                           "(nth (pixils.polygon/intersection "
                           "[{:x 0 :y 0} {:x 1.25 :y 0} {:x 1.25 :y 1.25} {:x 0 :y 1.25}] "
                           "[{:x 0.5 :y 0.5} {:x 2 :y 0.5} {:x 2 :y 2} {:x 0.5 :y 2}] "
                           "{:precision 3}) "
                           "0))");

  ASSERT_NE(area, nullptr);
  EXPECT_NEAR(area->f32(), 0.5625f, 0.0001f);
}

TEST_F(PolygonTest, polygon_boolean_ops_reject_invalid_precision)
{
  EXPECT_THROW(runtime.eval("(pixils.polygon/union [] {:precision 9})"), Roo::TypeError);
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
