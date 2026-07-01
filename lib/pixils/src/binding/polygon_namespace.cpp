#include "pixils/binding/polygon_namespace.h"

#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <roo/runtime/seq.h>

namespace Pixils::Script
{
  namespace
  {
    constexpr float EPSILON = 0.000001f;

    Roo::sptr_val bool_value(bool value)
    {
      return value ? Roo::Constant::BOOL_TRUE : Roo::Constant::BOOL_FALSE;
    }

    float cross(const Point& a, const Point& b, const Point& c)
    {
      return ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
    }

    bool nearly_zero(float value)
    {
      return std::abs(value) <= EPSILON;
    }

    bool point_on_segment(const Point& point, const Point& a, const Point& b)
    {
      if (!nearly_zero(cross(a, b, point))) return false;

      return point.x >= std::min(a.x, b.x) - EPSILON &&
             point.x <= std::max(a.x, b.x) + EPSILON &&
             point.y >= std::min(a.y, b.y) - EPSILON &&
             point.y <= std::max(a.y, b.y) + EPSILON;
    }

    bool rect_contains_point(const Rect& rect, const Point& point, bool include_boundary)
    {
      if (include_boundary)
      {
        return point.x >= rect.x && point.x <= rect.x + rect.w && point.y >= rect.y &&
               point.y <= rect.y + rect.h;
      }

      return rect.contains(point);
    }

    bool segment_intersects_segment(const Point& a,
                                    const Point& b,
                                    const Point& c,
                                    const Point& d)
    {
      const float ab_c = cross(a, b, c);
      const float ab_d = cross(a, b, d);
      const float cd_a = cross(c, d, a);
      const float cd_b = cross(c, d, b);

      if (nearly_zero(ab_c) && point_on_segment(c, a, b)) return true;
      if (nearly_zero(ab_d) && point_on_segment(d, a, b)) return true;
      if (nearly_zero(cd_a) && point_on_segment(a, c, d)) return true;
      if (nearly_zero(cd_b) && point_on_segment(b, c, d)) return true;

      return ((ab_c > 0.0f && ab_d < 0.0f) || (ab_c < 0.0f && ab_d > 0.0f)) &&
             ((cd_a > 0.0f && cd_b < 0.0f) || (cd_a < 0.0f && cd_b > 0.0f));
    }

    bool segments_cross_properly(const Point& a,
                                 const Point& b,
                                 const Point& c,
                                 const Point& d)
    {
      const float ab_c = cross(a, b, c);
      const float ab_d = cross(a, b, d);
      const float cd_a = cross(c, d, a);
      const float cd_b = cross(c, d, b);

      return ((ab_c > EPSILON && ab_d < -EPSILON) || (ab_c < -EPSILON && ab_d > EPSILON)) &&
             ((cd_a > EPSILON && cd_b < -EPSILON) || (cd_a < -EPSILON && cd_b > EPSILON));
    }

    bool polygon_edges_intersect(const std::vector<Point>& a,
                                 const std::vector<Point>& b,
                                 bool proper_only)
    {
      if (a.size() < 2 || b.size() < 2) return false;

      for (size_t i = 0; i < a.size(); i++)
      {
        const Point& a0 = a[i];
        const Point& a1 = a[(i + 1) % a.size()];
        for (size_t j = 0; j < b.size(); j++)
        {
          const Point& b0 = b[j];
          const Point& b1 = b[(j + 1) % b.size()];
          if (proper_only ? segments_cross_properly(a0, a1, b0, b1)
                          : segment_intersects_segment(a0, a1, b0, b1))
          {
            return true;
          }
        }
      }

      return false;
    }
  } // namespace

  namespace Geometry
  {
    std::vector<Point> points_from_value(const Roo::sptr_val& value)
    {
      std::vector<Point> points;
      Roo::sptr_val_v children = Roo::get_children(*value);
      points.reserve(children.size());
      for (auto& child : children)
      {
        points.push_back(Roo::obj<Point>(*child));
      }
      return points;
    }

    std::vector<Point> rect_points(const Rect& rect)
    {
      return {Point{rect.x, rect.y},
              Point{rect.x + rect.w, rect.y},
              Point{rect.x + rect.w, rect.y + rect.h},
              Point{rect.x, rect.y + rect.h}};
    }

    std::optional<Rect> polygon_bounds(const std::vector<Point>& points)
    {
      if (points.empty()) return std::nullopt;

      float min_x = points.front().x;
      float max_x = points.front().x;
      float min_y = points.front().y;
      float max_y = points.front().y;

      for (const Point& point : points)
      {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
      }

      const int x = static_cast<int>(std::floor(min_x));
      const int y = static_cast<int>(std::floor(min_y));
      const int right = static_cast<int>(std::ceil(max_x));
      const int bottom = static_cast<int>(std::ceil(max_y));
      return Rect{x, y, right - x, bottom - y};
    }

    Point segment_closest_point(const Point& a, const Point& b, const Point& point)
    {
      const float dx = b.x - a.x;
      const float dy = b.y - a.y;
      const float length_squared = dx * dx + dy * dy;
      if (length_squared <= EPSILON) return a;

      const float raw_t = ((point.x - a.x) * dx + (point.y - a.y) * dy) / length_squared;
      const float t = std::max(0.0f, std::min(1.0f, raw_t));
      return Point{a.x + t * dx, a.y + t * dy};
    }

    std::optional<Point> polygon_closest_edge_point(const std::vector<Point>& polygon,
                                                    const Point& point)
    {
      if (polygon.size() < 2) return std::nullopt;

      Point closest = segment_closest_point(polygon.front(), polygon[1], point);
      float closest_distance = closest.distance_squared_to(point);

      for (size_t i = 1; i < polygon.size(); i++)
      {
        const Point& a = polygon[i];
        const Point& b = polygon[(i + 1) % polygon.size()];
        const Point candidate = segment_closest_point(a, b, point);
        const float distance = candidate.distance_squared_to(point);
        if (distance < closest_distance)
        {
          closest = candidate;
          closest_distance = distance;
        }
      }

      return closest;
    }

    std::optional<Point> polygon_vertex_center(const std::vector<Point>& polygon)
    {
      if (polygon.empty()) return std::nullopt;

      float x = 0.0f;
      float y = 0.0f;
      for (const Point& point : polygon)
      {
        x += point.x;
        y += point.y;
      }

      const float size = static_cast<float>(polygon.size());
      return Point{x / size, y / size};
    }

    float polygon_area(const std::vector<Point>& polygon)
    {
      if (polygon.size() < 3) return 0.0f;

      float twice_area = 0.0f;
      for (size_t i = 0; i < polygon.size(); i++)
      {
        const Point& a = polygon[i];
        const Point& b = polygon[(i + 1) % polygon.size()];
        twice_area += a.x * b.y - b.x * a.y;
      }

      return std::abs(twice_area) * 0.5f;
    }

    bool rect_contains_rect(const Rect& outer, const Rect& inner)
    {
      return inner.x >= outer.x && inner.y >= outer.y &&
             inner.x + inner.w <= outer.x + outer.w &&
             inner.y + inner.h <= outer.y + outer.h;
    }

    bool rect_contains_polygon(const Rect& rect, const std::vector<Point>& polygon)
    {
      if (polygon.empty()) return false;
      return std::all_of(polygon.begin(),
                         polygon.end(),
                         [&rect](const Point& point) { return rect.contains(point); });
    }

    bool polygon_contains_point(const std::vector<Point>& polygon, const Point& point)
    {
      if (polygon.size() < 3) return false;

      bool inside = false;
      for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
      {
        const Point& a = polygon[j];
        const Point& b = polygon[i];

        if (point_on_segment(point, a, b)) return true;

        const bool crosses_y = (a.y > point.y) != (b.y > point.y);
        if (!crosses_y) continue;

        const float x_intersection = ((b.x - a.x) * (point.y - a.y) / (b.y - a.y)) + a.x;
        if (point.x < x_intersection)
        {
          inside = !inside;
        }
      }

      return inside;
    }

    bool polygon_contains_polygon(const std::vector<Point>& outer,
                                  const std::vector<Point>& inner)
    {
      if (outer.size() < 3 || inner.empty()) return false;

      const bool all_points_inside = std::all_of(
        inner.begin(),
        inner.end(),
        [&outer](const Point& point) { return polygon_contains_point(outer, point); });
      if (!all_points_inside) return false;

      return !polygon_edges_intersect(outer, inner, true);
    }

    bool polygons_intersect(const std::vector<Point>& a, const std::vector<Point>& b)
    {
      if (a.size() < 3 || b.size() < 3) return false;
      if (polygon_edges_intersect(a, b, false)) return true;
      return polygon_contains_point(a, b.front()) || polygon_contains_point(b, a.front());
    }

    bool rect_intersects_rect(const Rect& a, const Rect& b, bool include_boundary)
    {
      if (include_boundary)
      {
        return a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h && a.y + a.h >= b.y;
      }

      return a.intersects(b);
    }

    bool rect_intersects_polygon(const Rect& rect,
                                 const std::vector<Point>& polygon,
                                 bool include_boundary)
    {
      if (polygon.size() < 3) return false;

      auto bounds = polygon_bounds(polygon);
      if (!bounds || !rect_intersects_rect(rect, *bounds, include_boundary)) return false;

      const std::vector<Point> rect_polygon = rect_points(rect);
      if (std::any_of(polygon.begin(),
                      polygon.end(),
                      [&rect, include_boundary](const Point& point)
                      { return rect_contains_point(rect, point, include_boundary); }))
      {
        return true;
      }

      if (std::any_of(rect_polygon.begin(),
                      rect_polygon.end(),
                      [&polygon](const Point& point)
                      { return polygon_contains_point(polygon, point); }))
      {
        return true;
      }

      return polygon_edges_intersect(rect_polygon, polygon, false);
    }
  } // namespace Geometry

  namespace Function
  {
    FUNC_IMPL(PolygonBoundsFunction,
              SIG((FN_ARGS((&HostType::VECTOR_OF_POINT)),
                   EXEC_DISPATCH(&PolygonBoundsFunction::exec_bounds))));

    EXEC_BODY(PolygonBoundsFunction, exec_bounds)
    {
      std::optional<Rect> bounds =
        Geometry::polygon_bounds(Geometry::points_from_value(args[0]));
      if (!bounds) return Roo::Constant::NIL;
      return RectAdapter::make_unique(*bounds);
    }

    FUNC_IMPL(PolygonAreaFunction,
              SIG((FN_ARGS((&HostType::VECTOR_OF_POINT)),
                   EXEC_DISPATCH(&PolygonAreaFunction::exec_area))));

    EXEC_BODY(PolygonAreaFunction, exec_area)
    {
      return Roo::number(Geometry::polygon_area(Geometry::points_from_value(args[0])));
    }

    FUNC_IMPL(PolygonVertexCenterFunction,
              SIG((FN_ARGS((&HostType::VECTOR_OF_POINT)),
                   EXEC_DISPATCH(&PolygonVertexCenterFunction::exec_vertex_center))));

    EXEC_BODY(PolygonVertexCenterFunction, exec_vertex_center)
    {
      std::optional<Point> center =
        Geometry::polygon_vertex_center(Geometry::points_from_value(args[0]));
      if (!center) return Roo::Constant::NIL;
      return PointAdapter::make_unique(*center);
    }

    FUNC_IMPL(
      PolygonClosestEdgePointFunction,
      SIG((FN_ARGS((&HostType::VECTOR_OF_POINT), (&HostType::POINT)),
           EXEC_DISPATCH(&PolygonClosestEdgePointFunction::exec_closest_edge_point))));

    EXEC_BODY(PolygonClosestEdgePointFunction, exec_closest_edge_point)
    {
      std::optional<Point> closest =
        Geometry::polygon_closest_edge_point(Geometry::points_from_value(args[0]),
                                             Roo::obj<Point>(*args[1]));
      if (!closest) return Roo::Constant::NIL;
      return PointAdapter::make_unique(*closest);
    }

    FUNC_IMPL(PolygonContainsFunction,
              MULTI_SIG((FN_ARGS((&HostType::VECTOR_OF_POINT), (&HostType::RECT)),
                         EXEC_DISPATCH(&PolygonContainsFunction::exec_contains_rect)),
                        (FN_ARGS((&HostType::VECTOR_OF_POINT), (&HostType::POINT)),
                         EXEC_DISPATCH(&PolygonContainsFunction::exec_contains_point)),
                        (FN_ARGS((&HostType::VECTOR_OF_POINT), (&HostType::VECTOR_OF_POINT)),
                         EXEC_DISPATCH(&PolygonContainsFunction::exec_contains_polygon))));

    EXEC_BODY(PolygonContainsFunction, exec_contains_point)
    {
      return bool_value(
        Geometry::polygon_contains_point(Geometry::points_from_value(args[0]),
                                         Roo::obj<Point>(*args[1])));
    }

    EXEC_BODY(PolygonContainsFunction, exec_contains_rect)
    {
      return bool_value(
        Geometry::polygon_contains_polygon(Geometry::points_from_value(args[0]),
                                           Geometry::rect_points(Roo::obj<Rect>(*args[1]))));
    }

    EXEC_BODY(PolygonContainsFunction, exec_contains_polygon)
    {
      return bool_value(
        Geometry::polygon_contains_polygon(Geometry::points_from_value(args[0]),
                                           Geometry::points_from_value(args[1])));
    }

    FUNC_IMPL(
      PolygonIntersectsFunction,
      MULTI_SIG((FN_ARGS((&HostType::VECTOR_OF_POINT), (&HostType::RECT)),
                 EXEC_DISPATCH(&PolygonIntersectsFunction::exec_intersects_rect)),
                (FN_ARGS((&HostType::VECTOR_OF_POINT), (&HostType::VECTOR_OF_POINT)),
                 EXEC_DISPATCH(&PolygonIntersectsFunction::exec_intersects_polygon))));

    EXEC_BODY(PolygonIntersectsFunction, exec_intersects_rect)
    {
      return bool_value(
        Geometry::rect_intersects_polygon(Roo::obj<Rect>(*args[1]),
                                          Geometry::points_from_value(args[0])));
    }

    EXEC_BODY(PolygonIntersectsFunction, exec_intersects_polygon)
    {
      return bool_value(Geometry::polygons_intersect(Geometry::points_from_value(args[0]),
                                                     Geometry::points_from_value(args[1])));
    }
  } // namespace Function

  PolygonNamespace::PolygonNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__POLYGON))
  {
    values.emplace("area", Function::PolygonAreaFunction::make());
    values.emplace("bounds", Function::PolygonBoundsFunction::make());
    values.emplace("closest-edge-point", Function::PolygonClosestEdgePointFunction::make());
    values.emplace("contains?", Function::PolygonContainsFunction::make());
    values.emplace("intersects?", Function::PolygonIntersectsFunction::make());
    values.emplace("vertex-center", Function::PolygonVertexCenterFunction::make());
  }
} // namespace Pixils::Script
