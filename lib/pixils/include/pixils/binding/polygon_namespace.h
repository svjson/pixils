#ifndef PIXILS__POLYGON_NAMESPACE_H
#define PIXILS__POLYGON_NAMESPACE_H

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
    bool rect_contains_rect(const Rect& outer, const Rect& inner);
    bool rect_contains_polygon(const Rect& rect, const std::vector<Point>& polygon);
    bool rect_intersects_polygon(const Rect& rect, const std::vector<Point>& polygon);
    bool polygon_contains_point(const std::vector<Point>& polygon, const Point& point);
    bool polygon_contains_polygon(const std::vector<Point>& outer,
                                  const std::vector<Point>& inner);
    bool polygons_intersect(const std::vector<Point>& a, const std::vector<Point>& b);
  } // namespace Geometry

  namespace Function
  {
    /*! @brief Return the raster-oriented bounding rect of a polygon */
    FUNC(PolygonBoundsFunction, bounds);
    /*! @brief Test whether a polygon fully contains a point, rect, or polygon */
    FUNC(PolygonContainsFunction, contains_point, contains_rect, contains_polygon);
    /*! @brief Test whether a polygon intersects a rect or polygon */
    FUNC(PolygonIntersectsFunction, intersects_rect, intersects_polygon);
  } // namespace Function

  class PolygonNamespace : public Roo::Namespace
  {
   public:
    PolygonNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__POLYGON_NAMESPACE_H */
