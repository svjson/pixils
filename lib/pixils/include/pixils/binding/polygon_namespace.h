#ifndef PIXILS__POLYGON_NAMESPACE_H
#define PIXILS__POLYGON_NAMESPACE_H

#include <pixils/binding/point_namespace.h>
#include <pixils/geom.h>

#include <optional>
#include <roo/exec.h>
#include <roo/namespace.h>
#include <roo/runtime/value.h>
#include <string>
#include <vector>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__POLYGON = "pixils.polygon";

  namespace Geometry
  {
    std::vector<Point> points_from_value(const Roo::sptr_val& value);
    std::vector<Point> rect_points(const Rect& rect);
    std::optional<Rect> polygon_bounds(const std::vector<Point>& points);
    Point segment_closest_point(const Point& a, const Point& b, const Point& point);
    std::optional<Point> polygon_closest_edge_point(const std::vector<Point>& polygon,
                                                    const Point& point);
    std::optional<Point> polygon_vertex_center(const std::vector<Point>& polygon);
    float polygon_area(const std::vector<Point>& polygon);
    bool rect_contains_rect(const Rect& outer, const Rect& inner);
    bool rect_contains_polygon(const Rect& rect, const std::vector<Point>& polygon);
    bool rect_intersects_rect(const Rect& a, const Rect& b, bool include_boundary);
    bool rect_intersects_polygon(const Rect& rect,
                                 const std::vector<Point>& polygon,
                                 bool include_boundary = false);
    bool polygon_contains_point(const std::vector<Point>& polygon, const Point& point);
    bool polygon_contains_polygon(const std::vector<Point>& outer,
                                  const std::vector<Point>& inner);
    bool polygons_intersect(const std::vector<Point>& a, const std::vector<Point>& b);
    std::vector<std::vector<Point>> polygon_intersection(
      const std::vector<std::vector<Point>>& subjects,
      const std::vector<std::vector<Point>>& clips,
      int precision);
    std::vector<std::vector<Point>> polygon_union(
      const std::vector<std::vector<Point>>& polygons,
      int precision);
    std::vector<Point> circle_points(float cx,
                                     float cy,
                                     float radius,
                                     int segments,
                                     float rotation);
    std::vector<Point> ellipse_points(float cx,
                                      float cy,
                                      float rx,
                                      float ry,
                                      int segments,
                                      float rotation);
  } // namespace Geometry

  namespace Type
  {
    inline const Roo::SeqRef VECTOR_OF_POLYGON(&Roo::Type::VECTOR,
                                               &HostType::VECTOR_OF_POINT,
                                               "[[HPoint]]");
    inline const Roo::MultiRef POLYGON_INPUT({&HostType::VECTOR_OF_POINT,
                                              &VECTOR_OF_POLYGON},
                                             "[HPoint]|[[HPoint]]");
    inline const Roo::MultiRef POLYGON_INPUT_OR_MAP({&POLYGON_INPUT, &Roo::Type::MAP},
                                                    "[HPoint]|[[HPoint]]|Map");
  } // namespace Type

  namespace Function
  {
    /*! @brief Generate a polygon approximating a circle */
    FUNC(PolygonCircleFunction, circle, circle_with_opts);
    /*! @brief Generate a polygon approximating an ellipse */
    FUNC(PolygonEllipseFunction, ellipse, ellipse_with_opts);
    /*! @brief Return the raster-oriented bounding rect of a polygon */
    FUNC(PolygonBoundsFunction, bounds);
    /*! @brief Return the absolute area of a polygon */
    FUNC(PolygonAreaFunction, area);
    /*! @brief Return the average of the polygon vertices */
    FUNC(PolygonVertexCenterFunction, vertex_center);
    /*! @brief Return the closest point on any polygon edge */
    FUNC(PolygonClosestEdgePointFunction, closest_edge_point);
    /*! @brief Test whether a polygon fully contains a point, rect, or polygon */
    FUNC(PolygonContainsFunction, contains_point, contains_rect, contains_polygon);
    /*! @brief Test whether a polygon intersects a rect or polygon */
    FUNC(PolygonIntersectsFunction, intersects_rect, intersects_polygon);
    /*! @brief Return polygon intersections */
    FUNC(PolygonIntersectionFunction, intersection, intersection_with_opts);
    /*! @brief Return polygon union */
    FUNC(PolygonUnionFunction, combine);
  } // namespace Function

  class PolygonNamespace : public Roo::Namespace
  {
   public:
    PolygonNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__POLYGON_NAMESPACE_H */
