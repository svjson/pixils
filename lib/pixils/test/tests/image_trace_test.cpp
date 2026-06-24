#include <pixils/image_trace.h>

#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace
{
  using Pixils::ImageTrace::EdgePlacement;
  using Pixils::ImageTrace::StraightEdgeMask;
  using Pixils::ImageTrace::TraceOptions;

  void expect_point(const Pixils::Point& point, float x, float y)
  {
    EXPECT_FLOAT_EQ(point.x, x);
    EXPECT_FLOAT_EQ(point.y, y);
  }

  struct Bounds
  {
    float min_x;
    float min_y;
    float max_x;
    float max_y;

    bool operator==(const Bounds& other) const
    {
      return min_x == other.min_x && min_y == other.min_y && max_x == other.max_x &&
             max_y == other.max_y;
    }
  };

  Bounds bounds_of(const Pixils::ImageTrace::Polygon& polygon)
  {
    Bounds bounds{polygon.front().x,
                  polygon.front().y,
                  polygon.front().x,
                  polygon.front().y};
    for (const auto& point : polygon)
    {
      bounds.min_x = std::min(bounds.min_x, point.x);
      bounds.min_y = std::min(bounds.min_y, point.y);
      bounds.max_x = std::max(bounds.max_x, point.x);
      bounds.max_y = std::max(bounds.max_y, point.y);
    }
    return bounds;
  }
} // namespace

TEST(ImageTraceTest, trace_alpha_mask_returns_boundary_polygon_for_solid_region)
{
  std::vector<uint8_t> alpha = {
    255,
    255,
    255,
    255,
  };

  auto polygons = Pixils::ImageTrace::trace_alpha_mask(
    alpha,
    2,
    2,
    TraceOptions{.edge = EdgePlacement::BOUNDARY, .omit_straight_edges = {}});

  ASSERT_EQ(polygons.size(), 1u);
  ASSERT_EQ(polygons[0].size(), 4u);
  expect_point(polygons[0][0], 0.0f, 0.0f);
  expect_point(polygons[0][1], 2.0f, 0.0f);
  expect_point(polygons[0][2], 2.0f, 2.0f);
  expect_point(polygons[0][3], 0.0f, 2.0f);
}

TEST(ImageTraceTest, trace_alpha_mask_offsets_outer_edge_to_transparent_side_by_default)
{
  std::vector<uint8_t> alpha = {
    255,
    255,
    255,
    255,
  };

  auto polygons = Pixils::ImageTrace::trace_alpha_mask(alpha, 2, 2);

  ASSERT_EQ(polygons.size(), 1u);
  ASSERT_EQ(polygons[0].size(), 4u);
  expect_point(polygons[0][0], -0.5f, -0.5f);
  expect_point(polygons[0][1], 2.5f, -0.5f);
  expect_point(polygons[0][2], 2.5f, 2.5f);
  expect_point(polygons[0][3], -0.5f, 2.5f);
}

TEST(ImageTraceTest, trace_alpha_mask_can_offset_inner_edge_to_opaque_side)
{
  std::vector<uint8_t> alpha = {
    255,
    255,
    255,
    255,
  };

  auto polygons = Pixils::ImageTrace::trace_alpha_mask(
    alpha,
    2,
    2,
    TraceOptions{.edge = EdgePlacement::INNER, .omit_straight_edges = {}});

  ASSERT_EQ(polygons.size(), 1u);
  ASSERT_EQ(polygons[0].size(), 4u);
  expect_point(polygons[0][0], 0.5f, 0.5f);
  expect_point(polygons[0][1], 1.5f, 0.5f);
  expect_point(polygons[0][2], 1.5f, 1.5f);
  expect_point(polygons[0][3], 0.5f, 1.5f);
}

TEST(ImageTraceTest, trace_alpha_mask_returns_separate_polygons_for_separate_islands)
{
  std::vector<uint8_t> alpha = {255, 0, 255};

  auto polygons = Pixils::ImageTrace::trace_alpha_mask(
    alpha,
    3,
    1,
    TraceOptions{.edge = EdgePlacement::BOUNDARY, .omit_straight_edges = {}});

  ASSERT_EQ(polygons.size(), 2u);
  EXPECT_EQ(bounds_of(polygons[0]), (Bounds{0.0f, 0.0f, 1.0f, 1.0f}));
  EXPECT_EQ(bounds_of(polygons[1]), (Bounds{2.0f, 0.0f, 3.0f, 1.0f}));
}

TEST(ImageTraceTest, trace_alpha_mask_returns_transparent_hole_as_polygon)
{
  std::vector<uint8_t> alpha = {
    255,
    255,
    255,
    255,
    0,
    255,
    255,
    255,
    255,
  };

  auto polygons = Pixils::ImageTrace::trace_alpha_mask(
    alpha,
    3,
    3,
    TraceOptions{.edge = EdgePlacement::BOUNDARY, .omit_straight_edges = {}});

  ASSERT_EQ(polygons.size(), 2u);
  EXPECT_EQ(bounds_of(polygons[0]), (Bounds{0.0f, 0.0f, 3.0f, 3.0f}));
  EXPECT_EQ(bounds_of(polygons[1]), (Bounds{1.0f, 1.0f, 2.0f, 2.0f}));
}

TEST(ImageTraceTest, trace_alpha_mask_honors_alpha_threshold)
{
  std::vector<uint8_t> alpha = {
    32,
    255,
  };

  auto polygons =
    Pixils::ImageTrace::trace_alpha_mask(alpha,
                                         2,
                                         1,
                                         TraceOptions{.alpha_threshold = 128,
                                                      .edge = EdgePlacement::BOUNDARY,
                                                      .omit_straight_edges = {}});

  ASSERT_EQ(polygons.size(), 1u);
  EXPECT_EQ(bounds_of(polygons[0]), (Bounds{1.0f, 0.0f, 2.0f, 1.0f}));
}

TEST(ImageTraceTest, trace_alpha_mask_can_omit_straight_boundary_edge)
{
  std::vector<uint8_t> alpha = {
    255,
    255,
    255,
    255,
  };

  auto polygons = Pixils::ImageTrace::trace_alpha_mask(
    alpha,
    2,
    2,
    TraceOptions{.edge = EdgePlacement::BOUNDARY,
                 .omit_straight_edges = StraightEdgeMask{.north = true}});

  ASSERT_EQ(polygons.size(), 1u);
  ASSERT_EQ(polygons[0].size(), 4u);
  expect_point(polygons[0][0], 2.0f, 0.0f);
  expect_point(polygons[0][1], 2.0f, 2.0f);
  expect_point(polygons[0][2], 0.0f, 2.0f);
  expect_point(polygons[0][3], 0.0f, 0.0f);
}

TEST(ImageTraceTest, trace_alpha_mask_keeps_open_paths_when_boundary_edge_is_omitted)
{
  std::vector<uint8_t> alpha = {
    255,
    255,
    255,
    255,
  };

  auto polygons = Pixils::ImageTrace::trace_alpha_mask(
    alpha,
    2,
    2,
    TraceOptions{.edge = EdgePlacement::BOUNDARY,
                 .omit_straight_edges = StraightEdgeMask{.east = true}});

  ASSERT_EQ(polygons.size(), 1u);
  ASSERT_EQ(polygons[0].size(), 4u);
  expect_point(polygons[0][0], 2.0f, 2.0f);
  expect_point(polygons[0][1], 0.0f, 2.0f);
  expect_point(polygons[0][2], 0.0f, 0.0f);
  expect_point(polygons[0][3], 2.0f, 0.0f);
}
