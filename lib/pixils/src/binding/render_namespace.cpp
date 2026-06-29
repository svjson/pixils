
#include "pixils/binding/render_namespace.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>
#include <pixils/geom.h>

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <roo/host/schema.h>
#include <roo/namespace.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/exec_node.h>
#include <roo/runtime/lower.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>
#include <vector>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(CLOSE, "close");
    SHKEY(COLOR, "color");
    SHKEY(COLORS, "colors");
    SHKEY(FILL, "fill");
    SHKEY(FILL_STYLE, "fill-style");
    SHKEY(CLIP_RECT, "clip-rect");
    SHKEY(OFFSET, "offset");
    SHKEY(POS, "pos");
    SHKEY(REPEAT_X, "repeat-x?");
    SHKEY(REPEAT_Y, "repeat-y?");
    SHKEY(ROTATION, "rotation");
    SHKEY(SCALE, "scale");
    SHKEY(SOURCE, "source");
    SHKEY(STROKE_WIDTH, "stroke-width");
    SHKEY(TARGET, "target");
    SHKEY(FILL_STYLE_TYPE, "type");
    SHKEY(OPACITY, "opacity");
    SHKEY(BLEND_MODE, "blend-mode");
    SHKEY(FLIP_X, "flip-x?");
    SHKEY(FLIP_Y, "flip-y?");
  } // namespace MapKey

  namespace Function
  {
    namespace
    {
      constexpr double RADIANS_TO_DEGREES = 180.0 / 3.14159265358979323846;

      Uint8 opacity_to_alpha(float opacity)
      {
        return static_cast<Uint8>(std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
      }

      bool dict_contains(const Roo::sptr_val& value, const std::string& key)
      {
        return value && value->type == Roo::Value::Type::MAP &&
               Roo::Dict::contains_key(*value, key);
      }

      bool image_options_map(const Roo::sptr_val& value)
      {
        if (!value || value->type != Roo::Value::Type::MAP) return false;

        return dict_contains(value, std::get<std::string>(MapKey::POS->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::TARGET->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::CLIP_RECT->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::SCALE->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::SOURCE->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::OPACITY->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::BLEND_MODE->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::ROTATION->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::FLIP_X->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::FLIP_Y->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::REPEAT_X->value)) ||
               dict_contains(value, std::get<std::string>(MapKey::REPEAT_Y->value));
      }

      bool rect_like_map(const Roo::sptr_val& value)
      {
        return dict_contains(value, "w") || dict_contains(value, "h");
      }

      bool seq_value(const Roo::sptr_val& value)
      {
        return value && (value->type == Roo::Value::Type::VECTOR ||
                         value->type == Roo::Value::Type::LIST);
      }

      Color color_from_value(Roo::Context& ctx,
                             Roo::sptr_val value,
                             const std::string& context)
      {
        if (!value)
        {
          throw Roo::TypeError(context + " must be a color.");
        }

        if (!HostType::COLOR.is_type_of(*value))
        {
          auto coercion = HostType::COLOR.coerce(ctx, value);
          if (!coercion.success)
          {
            throw Roo::TypeError(context + " must be a color.");
          }
          value = coercion.result;
        }

        return Roo::obj<Color>(*value);
      }

      Uint8 interpolated_channel(float a, float b, float c, float wa, float wb, float wc)
      {
        const float value = (a * wa) + (b * wb) + (c * wc);
        return static_cast<Uint8>(std::lround(std::clamp(value, 0.0f, 255.0f)));
      }

      Color interpolated_color(const Color& a,
                               const Color& b,
                               const Color& c,
                               float wa,
                               float wb,
                               float wc)
      {
        return Color{interpolated_channel(a.r, b.r, c.r, wa, wb, wc),
                     interpolated_channel(a.g, b.g, c.g, wa, wb, wc),
                     interpolated_channel(a.b, b.b, c.b, wa, wb, wc),
                     interpolated_channel(a.a, b.a, c.a, wa, wb, wc)};
      }

      void fill_horizontal_span(SDL_Renderer* renderer, int x1, int x2, int y)
      {
        if (x2 < x1) std::swap(x1, x2);
        SDL_Rect rect{x1, y, x2 - x1 + 1, 1};
        SDL_RenderFillRect(renderer, &rect);
      }

      void fill_pixel(SDL_Renderer* renderer, int x, int y)
      {
        SDL_Rect rect{x, y, 1, 1};
        SDL_RenderFillRect(renderer, &rect);
      }

      void fill_pixel(SDL_Renderer* renderer, int x, int y, const Color& color)
      {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        fill_pixel(renderer, x, y);
      }

      void draw_circle_outline(SDL_Renderer* renderer, int cx, int cy, int radius)
      {
        int x = 0;
        int y = radius;
        int d = 3 - (2 * radius);

        while (y >= x)
        {
          fill_pixel(renderer, cx + x, cy + y);
          fill_pixel(renderer, cx - x, cy + y);
          fill_pixel(renderer, cx + x, cy - y);
          fill_pixel(renderer, cx - x, cy - y);
          fill_pixel(renderer, cx + y, cy + x);
          fill_pixel(renderer, cx - y, cy + x);
          fill_pixel(renderer, cx + y, cy - x);
          fill_pixel(renderer, cx - y, cy - x);

          x++;
          if (d > 0)
          {
            y--;
            d = d + (4 * (x - y)) + 10;
          }
          else
          {
            d = d + (4 * x) + 6;
          }
        }
      }

      void draw_filled_circle(SDL_Renderer* renderer, int cx, int cy, int radius)
      {
        const int radius_squared = radius * radius;
        for (int y = -radius; y <= radius; y++)
        {
          const int x = static_cast<int>(
            std::floor(std::sqrt(static_cast<double>(radius_squared - (y * y)))));
          fill_horizontal_span(renderer, cx - x, cx + x, cy + y);
        }
      }

      int ellipse_x_for_y(int rx, int ry, int y)
      {
        if (ry == 0) return rx;
        const double normalized_y = static_cast<double>(y) / static_cast<double>(ry);
        const double remaining = std::max(0.0, 1.0 - (normalized_y * normalized_y));
        return static_cast<int>(std::floor(static_cast<double>(rx) * std::sqrt(remaining)));
      }

      int ellipse_y_for_x(int rx, int ry, int x)
      {
        if (rx == 0) return ry;
        const double normalized_x = static_cast<double>(x) / static_cast<double>(rx);
        const double remaining = std::max(0.0, 1.0 - (normalized_x * normalized_x));
        return static_cast<int>(std::floor(static_cast<double>(ry) * std::sqrt(remaining)));
      }

      void draw_filled_ellipse(SDL_Renderer* renderer, int cx, int cy, int rx, int ry)
      {
        if (ry == 0)
        {
          fill_horizontal_span(renderer, cx - rx, cx + rx, cy);
          return;
        }

        for (int y = -ry; y <= ry; y++)
        {
          const int x = ellipse_x_for_y(rx, ry, y);
          fill_horizontal_span(renderer, cx - x, cx + x, cy + y);
        }
      }

      void draw_ellipse_outline(SDL_Renderer* renderer, int cx, int cy, int rx, int ry)
      {
        if (rx == 0 && ry == 0)
        {
          fill_pixel(renderer, cx, cy);
          return;
        }
        if (ry == 0)
        {
          fill_horizontal_span(renderer, cx - rx, cx + rx, cy);
          return;
        }
        if (rx == 0)
        {
          for (int y = -ry; y <= ry; y++)
          {
            fill_pixel(renderer, cx, cy + y);
          }
          return;
        }

        for (int y = -ry; y <= ry; y++)
        {
          const int x = ellipse_x_for_y(rx, ry, y);
          fill_pixel(renderer, cx + x, cy + y);
          fill_pixel(renderer, cx - x, cy + y);
        }

        for (int x = -rx; x <= rx; x++)
        {
          const int y = ellipse_y_for_x(rx, ry, x);
          fill_pixel(renderer, cx + x, cy + y);
          fill_pixel(renderer, cx + x, cy - y);
        }
      }

      template <typename Points>
      void draw_filled_polygon(SDL_Renderer* renderer,
                               const Points& points,
                               std::vector<float>& intersections)
      {
        if (points.size() < 3) return;

        float min_y = points.front().y;
        float max_y = points.front().y;
        for (const Point& point : points)
        {
          min_y = std::min(min_y, point.y);
          max_y = std::max(max_y, point.y);
        }

        const int first_y = static_cast<int>(std::floor(min_y));
        const int last_y = static_cast<int>(std::ceil(max_y)) - 1;
        intersections.reserve(points.size());

        for (int y = first_y; y <= last_y; y++)
        {
          const float scan_y = static_cast<float>(y) + 0.5f;
          intersections.clear();

          for (size_t i = 0; i < points.size(); i++)
          {
            const Point& a = points[i];
            const Point& b = points[(i + 1) % points.size()];
            const float edge_min_y = std::min(a.y, b.y);
            const float edge_max_y = std::max(a.y, b.y);

            if (edge_min_y == edge_max_y || scan_y < edge_min_y || scan_y >= edge_max_y)
            {
              continue;
            }

            const float t = (scan_y - a.y) / (b.y - a.y);
            intersections.push_back(a.x + (t * (b.x - a.x)));
          }

          std::sort(intersections.begin(), intersections.end());
          for (size_t i = 0; i + 1 < intersections.size(); i += 2)
          {
            const int x1 = static_cast<int>(std::ceil(intersections[i] - 0.5f));
            const int x2 = static_cast<int>(
              std::floor(std::nextafter(intersections[i + 1] - 0.5f,
                                        -std::numeric_limits<float>::infinity())));
            if (x2 >= x1)
            {
              fill_horizontal_span(renderer, x1, x2, y);
            }
          }
        }
      }

      struct VertexColoredPoint
      {
        Point point;
        Color color;
      };

      struct BarycentricWeights
      {
        float a = 0.0f;
        float b = 0.0f;
        float c = 0.0f;
      };

      std::optional<BarycentricWeights> barycentric_weights(const Point& p,
                                                            const Point& a,
                                                            const Point& b,
                                                            const Point& c)
      {
        const float denominator = ((b.y - c.y) * (a.x - c.x)) + ((c.x - b.x) * (a.y - c.y));
        if (std::abs(denominator) <= std::numeric_limits<float>::epsilon())
        {
          return std::nullopt;
        }

        BarycentricWeights weights;
        weights.a =
          (((b.y - c.y) * (p.x - c.x)) + ((c.x - b.x) * (p.y - c.y))) / denominator;
        weights.b =
          (((c.y - a.y) * (p.x - c.x)) + ((a.x - c.x) * (p.y - c.y))) / denominator;
        weights.c = 1.0f - weights.a - weights.b;

        constexpr float EPSILON = -0.00001f;
        if (weights.a < EPSILON || weights.b < EPSILON || weights.c < EPSILON)
        {
          return std::nullopt;
        }

        return weights;
      }

      bool draw_vertex_colored_triangle_pixel(SDL_Renderer* renderer,
                                              int x,
                                              int y,
                                              const VertexColoredPoint& a,
                                              const VertexColoredPoint& b,
                                              const VertexColoredPoint& c)
      {
        const Point pixel_center{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
        auto weights = barycentric_weights(pixel_center, a.point, b.point, c.point);
        if (!weights) return false;

        fill_pixel(
          renderer,
          x,
          y,
          interpolated_color(a.color, b.color, c.color, weights->a, weights->b, weights->c));
        return true;
      }

      float cross_product(const Point& a, const Point& b, const Point& c)
      {
        return ((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x));
      }

      float signed_polygon_area(const std::vector<Point>& points)
      {
        float area = 0.0f;
        for (size_t i = 0; i < points.size(); i++)
        {
          const Point& a = points[i];
          const Point& b = points[(i + 1) % points.size()];
          area += (a.x * b.y) - (b.x * a.y);
        }
        return area * 0.5f;
      }

      bool point_in_triangle(const Point& p, const Point& a, const Point& b, const Point& c)
      {
        return barycentric_weights(p, a, b, c).has_value();
      }

      std::vector<std::array<size_t, 3>> triangulate_polygon(
        const std::vector<Point>& points)
      {
        std::vector<std::array<size_t, 3>> triangles;
        if (points.size() < 3) return triangles;

        const float area = signed_polygon_area(points);
        if (std::abs(area) <= std::numeric_limits<float>::epsilon()) return triangles;

        const bool ccw = area > 0.0f;
        std::vector<size_t> remaining;
        remaining.reserve(points.size());
        for (size_t i = 0; i < points.size(); i++)
        {
          remaining.push_back(i);
        }

        size_t guard = 0;
        while (remaining.size() > 3 && guard < points.size() * points.size())
        {
          bool clipped = false;
          for (size_t i = 0; i < remaining.size(); i++)
          {
            const size_t prev_index =
              remaining[(i + remaining.size() - 1) % remaining.size()];
            const size_t curr_index = remaining[i];
            const size_t next_index = remaining[(i + 1) % remaining.size()];

            const Point& prev = points[prev_index];
            const Point& curr = points[curr_index];
            const Point& next = points[next_index];
            const float cross = cross_product(prev, curr, next);
            if ((ccw && cross <= 0.00001f) || (!ccw && cross >= -0.00001f))
            {
              continue;
            }

            bool contains_point = false;
            for (size_t test_index : remaining)
            {
              if (test_index == prev_index || test_index == curr_index ||
                  test_index == next_index)
              {
                continue;
              }

              if (point_in_triangle(points[test_index], prev, curr, next))
              {
                contains_point = true;
                break;
              }
            }

            if (contains_point) continue;

            triangles.push_back({prev_index, curr_index, next_index});
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
            clipped = true;
            break;
          }

          if (!clipped) break;
          guard++;
        }

        if (remaining.size() == 3)
        {
          triangles.push_back({remaining[0], remaining[1], remaining[2]});
        }

        return triangles;
      }

      void draw_filled_vertex_colored_polygon(SDL_Renderer* renderer,
                                              const std::vector<Point>& points,
                                              const std::vector<Color>& colors)
      {
        if (points.size() != colors.size() || points.size() < 3) return;

        std::vector<std::array<size_t, 3>> triangles = triangulate_polygon(points);
        if (triangles.empty()) return;
        if (triangles.size() != points.size() - 2)
        {
          throw Roo::TypeError(
            "polygon!: could not triangulate :fill-style :vertex-colors polygon.");
        }

        std::vector<VertexColoredPoint> vertices;
        vertices.reserve(points.size());
        for (size_t i = 0; i < points.size(); i++)
        {
          vertices.push_back(VertexColoredPoint{points[i], colors[i]});
        }

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

        const int first_x = static_cast<int>(std::floor(min_x));
        const int last_x = static_cast<int>(std::ceil(max_x)) - 1;
        const int first_y = static_cast<int>(std::floor(min_y));
        const int last_y = static_cast<int>(std::ceil(max_y)) - 1;

        for (int y = first_y; y <= last_y; y++)
        {
          for (int x = first_x; x <= last_x; x++)
          {
            for (const auto& triangle : triangles)
            {
              if (draw_vertex_colored_triangle_pixel(renderer,
                                                     x,
                                                     y,
                                                     vertices[triangle[0]],
                                                     vertices[triangle[1]],
                                                     vertices[triangle[2]]))
              {
                break;
              }
            }
          }
        }
      }

      void draw_filled_quad(SDL_Renderer* renderer, const std::array<Point, 4>& points)
      {
        float min_y = points.front().y;
        float max_y = points.front().y;
        for (const Point& point : points)
        {
          min_y = std::min(min_y, point.y);
          max_y = std::max(max_y, point.y);
        }

        const int first_y = static_cast<int>(std::floor(min_y));
        const int last_y = static_cast<int>(std::ceil(max_y)) - 1;
        std::array<float, 4> intersections{};

        for (int y = first_y; y <= last_y; y++)
        {
          const float scan_y = static_cast<float>(y) + 0.5f;
          size_t intersection_count = 0;

          for (size_t i = 0; i < points.size(); i++)
          {
            const Point& a = points[i];
            const Point& b = points[(i + 1) % points.size()];
            const float edge_min_y = std::min(a.y, b.y);
            const float edge_max_y = std::max(a.y, b.y);

            if (edge_min_y == edge_max_y || scan_y < edge_min_y || scan_y >= edge_max_y)
            {
              continue;
            }

            const float t = (scan_y - a.y) / (b.y - a.y);
            intersections[intersection_count++] = a.x + (t * (b.x - a.x));
          }

          for (size_t i = 1; i < intersection_count; i++)
          {
            float value = intersections[i];
            size_t j = i;
            while (j > 0 && intersections[j - 1] > value)
            {
              intersections[j] = intersections[j - 1];
              j--;
            }
            intersections[j] = value;
          }

          for (size_t i = 0; i + 1 < intersection_count; i += 2)
          {
            const int x1 = static_cast<int>(std::ceil(intersections[i] - 0.5f));
            const int x2 = static_cast<int>(
              std::floor(std::nextafter(intersections[i + 1] - 0.5f,
                                        -std::numeric_limits<float>::infinity())));
            if (x2 >= x1)
            {
              fill_horizontal_span(renderer, x1, x2, y);
            }
          }
        }
      }

      std::array<Point, 4> stroked_segment_quad(const Point& from,
                                                const Point& to,
                                                float stroke_width)
      {
        const float half_width = stroke_width * 0.5f;
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float length = std::sqrt((dx * dx) + (dy * dy));
        const Point from_center{from.x + 0.5f, from.y + 0.5f};
        const Point to_center{to.x + 0.5f, to.y + 0.5f};

        if (length <= std::numeric_limits<float>::epsilon())
        {
          return {Point{from_center.x - half_width, from_center.y - half_width},
                  Point{from_center.x + half_width, from_center.y - half_width},
                  Point{from_center.x + half_width, from_center.y + half_width},
                  Point{from_center.x - half_width, from_center.y + half_width}};
        }

        const float ux = dx / length;
        const float uy = dy / length;
        const float nx = -uy * half_width;
        const float ny = ux * half_width;
        const Point start{from_center.x - (ux * 0.5f), from_center.y - (uy * 0.5f)};
        const Point end{to_center.x + (ux * 0.5f), to_center.y + (uy * 0.5f)};

        return {Point{start.x + nx, start.y + ny},
                Point{end.x + nx, end.y + ny},
                Point{end.x - nx, end.y - ny},
                Point{start.x - nx, start.y - ny}};
      }

      void draw_stroked_segment(SDL_Renderer* renderer,
                                const Point& from,
                                const Point& to,
                                float stroke_width)
      {
        if (stroke_width <= 0.0f) return;

        if (stroke_width <= 1.0f)
        {
          SDL_RenderDrawLine(renderer,
                             from.round_x(),
                             from.round_y(),
                             to.round_x(),
                             to.round_y());
          return;
        }

        const auto quad = stroked_segment_quad(from, to, stroke_width);
        draw_filled_quad(renderer, quad);
      }

      void draw_stroked_polyline(SDL_Renderer* renderer,
                                 const std::vector<Point>& points,
                                 bool close_shape,
                                 float stroke_width)
      {
        if (stroke_width <= 0.0f || points.size() < 2) return;

        for (size_t i = 0; i + 1 < points.size(); i++)
        {
          draw_stroked_segment(renderer, points[i], points[i + 1], stroke_width);
        }

        if (close_shape)
        {
          draw_stroked_segment(renderer, points.back(), points.front(), stroke_width);
        }
      }

      struct PolygonFillStyle
      {
        enum class Type
        {
          SOLID,
          VERTEX_COLORS
        };

        Type type = Type::SOLID;
        std::optional<Color> color = std::nullopt;
        std::vector<Color> colors;
      };

      std::string required_keyword_property(const Roo::sptr_val& map,
                                            const std::string& key,
                                            const std::string& context)
      {
        Roo::sptr_val value = Roo::Dict::get_property(*map, key);
        if (!value || value->type == Roo::Value::Type::NIL)
        {
          throw Roo::TypeError(context + " requires :" + key + ".");
        }

        if (value->type != Roo::Value::Type::KEYWORD)
        {
          throw Roo::TypeError(context + " :" + key + " must be a keyword.");
        }

        return value->str();
      }

      Roo::sptr_val required_property(const Roo::sptr_val& map,
                                      const std::string& key,
                                      const std::string& context)
      {
        Roo::sptr_val value = Roo::Dict::get_property(*map, key);
        if (!value || value->type == Roo::Value::Type::NIL)
        {
          throw Roo::TypeError(context + " requires :" + key + ".");
        }
        return value;
      }

      PolygonFillStyle parse_polygon_fill_style(Roo::Context& ctx,
                                                const Roo::sptr_val& value)
      {
        if (!value || value->type != Roo::Value::Type::MAP)
        {
          throw Roo::TypeError("polygon!: :fill-style must be a map.");
        }

        const std::string type =
          required_keyword_property(value,
                                    std::get<std::string>(MapKey::FILL_STYLE_TYPE->value),
                                    "polygon!: :fill-style");

        if (type == "solid")
        {
          PolygonFillStyle style;
          style.type = PolygonFillStyle::Type::SOLID;
          style.color =
            color_from_value(ctx,
                             required_property(value,
                                               std::get<std::string>(MapKey::COLOR->value),
                                               "polygon!: :fill-style :solid"),
                             "polygon!: :fill-style :solid :color");
          return style;
        }

        if (type == "vertex-colors")
        {
          Roo::sptr_val colors_value =
            required_property(value,
                              std::get<std::string>(MapKey::COLORS->value),
                              "polygon!: :fill-style :vertex-colors");
          if (!seq_value(colors_value))
          {
            throw Roo::TypeError(
              "polygon!: :fill-style :vertex-colors :colors must be a sequence.");
          }

          PolygonFillStyle style;
          style.type = PolygonFillStyle::Type::VERTEX_COLORS;
          for (auto& color_value : Roo::get_children(*colors_value))
          {
            style.colors.push_back(
              color_from_value(ctx,
                               color_value,
                               "polygon!: :fill-style :vertex-colors :colors entry"));
          }
          return style;
        }

        throw Roo::TypeError("polygon!: unsupported :fill-style :type: " + type);
      }

      Uint8 image_opacity_alpha(Roo::MapSchema::Inspector& opts)
      {
        if (opts.contains(std::get<std::string>(MapKey::OPACITY->value)))
        {
          return opacity_to_alpha(opts.f32(std::get<std::string>(MapKey::OPACITY->value)));
        }
        return 255;
      }

      SDL_BlendMode erase_alpha_blend_mode()
      {
        static SDL_BlendMode mode =
          SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO,
                                     SDL_BLENDFACTOR_ONE,
                                     SDL_BLENDOPERATION_ADD,
                                     SDL_BLENDFACTOR_ZERO,
                                     SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                     SDL_BLENDOPERATION_ADD);
        return mode;
      }

      SDL_BlendMode image_blend_mode(Roo::MapSchema::Inspector& opts)
      {
        auto value = opts.val(std::get<std::string>(MapKey::BLEND_MODE->value));
        if (!value || value->type == Roo::Value::Type::NIL) return SDL_BLENDMODE_BLEND;
        if (value->type != Roo::Value::Type::KEYWORD)
        {
          throw Roo::TypeError("image!: :blend-mode must be a keyword.");
        }

        std::string mode = value->str();
        if (mode == "blend") return SDL_BLENDMODE_BLEND;
        if (mode == "none") return SDL_BLENDMODE_NONE;
        if (mode == "erase-alpha") return erase_alpha_blend_mode();

        throw Roo::TypeError("image!: unsupported :blend-mode: " + mode);
      }

      std::optional<Rect> intersect_clip_rect(const std::optional<Rect>& current,
                                              const Rect& requested)
      {
        if (!current) return requested;

        int x1 = std::max(current->x, requested.x);
        int y1 = std::max(current->y, requested.y);
        int x2 = std::min(current->x + current->w, requested.x + requested.w);
        int y2 = std::min(current->y + current->h, requested.y + requested.h);

        return Rect{x1, y1, std::max(0, x2 - x1), std::max(0, y2 - y1)};
      }

      std::optional<Rect> visible_clip_rect(const std::optional<Rect>& current,
                                            const Rect& requested)
      {
        auto clip = intersect_clip_rect(current, requested);
        if (!clip || clip->w <= 0 || clip->h <= 0) return std::nullopt;
        return clip;
      }

      struct ImageTarget
      {
        Rect rect;
      };

      ImageTarget image_target_from_value(Roo::Context& ctx,
                                          Roo::sptr_val value,
                                          int default_width,
                                          int default_height,
                                          float scale)
      {
        if (!value || value->type == Roo::Value::Type::NIL)
        {
          throw Roo::TypeError("image!: options require :target or :pos.");
        }

        if (HostType::RECT.is_type_of(*value))
        {
          const Rect& rect = Roo::obj<Rect>(*value);
          return {rect};
        }

        if (HostType::POINT.is_type_of(*value))
        {
          const Point& point = Roo::obj<Point>(*value);
          return {Rect{point.round_x(),
                       point.round_y(),
                       static_cast<int>(std::round(default_width * scale)),
                       static_cast<int>(std::round(default_height * scale))}};
        }

        if (rect_like_map(value))
        {
          auto rect_value = value;
          auto coercion = HostType::RECT.coerce(ctx, rect_value);
          if (!coercion.success)
          {
            throw Roo::TypeError("image!: :target rect must be a Rect or rect map.");
          }
          const Rect& rect = Roo::obj<Rect>(*coercion.result);
          return {rect};
        }

        auto point_value = value;
        auto coercion = HostType::POINT.coerce(ctx, point_value);
        if (!coercion.success)
        {
          throw Roo::TypeError("image!: :target must be a Point, Rect, or map.");
        }
        const Point& point = Roo::obj<Point>(*coercion.result);
        return {Rect{point.round_x(),
                     point.round_y(),
                     static_cast<int>(std::round(default_width * scale)),
                     static_cast<int>(std::round(default_height * scale))}};
      }

      int repeat_start(int anchor, int size, int min)
      {
        if (size <= 0) return anchor;
        double steps =
          std::floor(static_cast<double>(min - anchor) / static_cast<double>(size));
        return anchor + static_cast<int>(steps) * size;
      }

      void render_image_copy(SDL_Renderer* renderer,
                             SDL_Texture* texture,
                             const SDL_Rect* source,
                             const SDL_Rect& dest,
                             float rotation,
                             SDL_RendererFlip flip)
      {
        if (rotation == 0.0f && flip == SDL_FLIP_NONE)
        {
          SDL_RenderCopy(renderer, texture, source, &dest);
        }
        else
        {
          SDL_RenderCopyEx(renderer,
                           texture,
                           source,
                           &dest,
                           static_cast<double>(rotation) * RADIANS_TO_DEGREES,
                           nullptr,
                           flip);
        }
      }

      void render_image_tiles(SDL_Renderer* renderer,
                              SDL_Texture* texture,
                              const SDL_Rect* source,
                              const SDL_Rect& target,
                              const Rect& bounds,
                              bool repeat_x,
                              bool repeat_y,
                              float rotation,
                              SDL_RendererFlip flip)
      {
        if (target.w <= 0 || target.h <= 0) return;

        int start_x = repeat_x ? repeat_start(target.x, target.w, bounds.x) : target.x;
        int start_y = repeat_y ? repeat_start(target.y, target.h, bounds.y) : target.y;
        int end_x = repeat_x ? bounds.x + bounds.w : target.x + 1;
        int end_y = repeat_y ? bounds.y + bounds.h : target.y + 1;

        for (int y = start_y; y < end_y; y += target.h)
        {
          for (int x = start_x; x < end_x; x += target.w)
          {
            SDL_Rect dest{x, y, target.w, target.h};
            render_image_copy(renderer, texture, source, dest, rotation, flip);
          }
        }
      }
    } // namespace

    FUNC_IMPL(DrawImageBang,
              MULTI_SIG((FN_ARGS((&Roo::Type::KEYWORD), (&HostType::POINT)),
                         EXEC_DISPATCH(&DrawImageBang::exec_draw_img)),
                        (FN_ARGS((&Roo::Type::KEYWORD), (&HostType::RECT)),
                         EXEC_DISPATCH(&DrawImageBang::exec_draw_img)),
                        (FN_ARGS((&Roo::Type::KEYWORD), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&DrawImageBang::exec_draw_img))));

    EXEC_BODY(DrawImageBang, exec_draw_img)
    {
      static Roo::MapSchema draw_image_opts_schema({},
                                                   {{"pos", &HostType::POINT},
                                                    {"target", &Roo::Type::ANY},
                                                    {"clip-rect", &HostType::RECT},
                                                    {"scale", &Roo::Type::NUMBER},
                                                    {"opacity", &Roo::Type::NUMBER},
                                                    {"blend-mode", &Roo::Type::KEYWORD},
                                                    {"rotation", &Roo::Type::NUMBER},
                                                    {"source", &HostType::RECT},
                                                    {"repeat-x?", &Roo::Type::BOOL},
                                                    {"repeat-y?", &Roo::Type::BOOL},
                                                    {"flip-x?", &Roo::Type::BOOL},
                                                    {"flip-y?", &Roo::Type::BOOL}});

      if (args[0]->type == Roo::Value::Type::KEYWORD)
      {
        auto [asset_bundle, asset_key] = args.front()->qual();

        /**
         * Force detection of Point-arg, as coercion will not have happened during
         * for map-shaped Points during dispatch
         */
        Roo::sptr_val map_arg =
          image_options_map(args[1])
            ? args[1]
            : Roo::map(
                {Roo::keyword(std::get<std::string>(MapKey::TARGET->value)), args[1]});

        auto opts = draw_image_opts_schema.bind(ctx, *map_arg);

        RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

        SDL_Texture* texture = rc.asset_registry->get_image(asset_bundle, asset_key);
        if (!texture) return Roo::Constant::NIL;

        float scale = opts.f32("scale", 1.0f);
        float rotation = opts.f32(std::get<std::string>(MapKey::ROTATION->value), 0.0f);
        bool flip_x = opts.boolean(std::get<std::string>(MapKey::FLIP_X->value), false);
        bool flip_y = opts.boolean(std::get<std::string>(MapKey::FLIP_Y->value), false);
        bool repeat_x = opts.boolean(std::get<std::string>(MapKey::REPEAT_X->value), false);
        bool repeat_y = opts.boolean(std::get<std::string>(MapKey::REPEAT_Y->value), false);
        Uint8 alpha = image_opacity_alpha(opts);
        SDL_BlendMode blend_mode = image_blend_mode(opts);
        std::optional<SDL_Rect> source_rect = std::nullopt;
        if (auto source = opts.val(std::get<std::string>(MapKey::SOURCE->value));
            source && source->type != Roo::Value::Type::NIL)
        {
          source_rect =
            opts.obj<Rect>(std::get<std::string>(MapKey::SOURCE->value)).to_SDL_rect();
        }

        int source_width = 0;
        int source_height = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &source_width, &source_height);
        if (source_rect)
        {
          source_width = source_rect->w;
          source_height = source_rect->h;
        }

        Roo::sptr_val target_value =
          opts.contains(std::get<std::string>(MapKey::TARGET->value))
            ? opts.val(std::get<std::string>(MapKey::TARGET->value))
            : opts.val(std::get<std::string>(MapKey::POS->value));
        ImageTarget target =
          image_target_from_value(ctx, target_value, source_width, source_height, scale);
        SDL_Rect dest = target.rect.to_SDL_rect();

        const SDL_Rect* source_ptr = source_rect ? &*source_rect : nullptr;
        SDL_RendererFlip flip =
          static_cast<SDL_RendererFlip>((flip_x ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE) |
                                        (flip_y ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE));

        std::optional<Rect> previous_clip = rc.current_clip_rect;
        std::optional<Rect> requested_clip =
          opts.optional_obj<Rect>(std::get<std::string>(MapKey::CLIP_RECT->value));
        std::optional<Rect> effective_bounds;
        if (requested_clip)
        {
          effective_bounds = visible_clip_rect(previous_clip, *requested_clip);
          if (!effective_bounds) return Roo::Constant::NIL;
          rc.set_clip_rect(effective_bounds);
        }
        else if ((repeat_x || repeat_y) && previous_clip)
        {
          effective_bounds = previous_clip;
        }
        else
        {
          effective_bounds = target.rect;
        }

        if (blend_mode != SDL_BLENDMODE_BLEND)
        {
          SDL_SetTextureBlendMode(texture, blend_mode);
        }
        SDL_SetTextureAlphaMod(texture, alpha);
        try
        {
          render_image_tiles(rc.renderer,
                             texture,
                             source_ptr,
                             dest,
                             *effective_bounds,
                             repeat_x,
                             repeat_y,
                             rotation,
                             flip);
        }
        catch (...)
        {
          if (alpha != 255) SDL_SetTextureAlphaMod(texture, 255);
          if (blend_mode != SDL_BLENDMODE_BLEND)
          {
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
          }
          if (requested_clip) rc.set_clip_rect(previous_clip);
          throw;
        }
        if (alpha != 255) SDL_SetTextureAlphaMod(texture, 255);
        if (blend_mode != SDL_BLENDMODE_BLEND)
        {
          SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        }
        if (requested_clip) rc.set_clip_rect(previous_clip);
      }

      return Roo::Constant::NIL;
    }

    /* DrawLineBang - line! */
    FUNC_IMPL(DrawLineBang,
              MULTI_SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                         EXEC_DISPATCH(&DrawLineBang::exec_draw_line)),
                        (FN_ARGS((&HostType::POINT), (&HostType::POINT), (&Roo::Type::ANY)),
                         EXEC_DISPATCH(&DrawLineBang::exec_draw_line))));

    EXEC_BODY(DrawLineBang, exec_draw_line)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      const Point& from = Roo::obj<Point>(*args[0]);
      const Point& to = Roo::obj<Point>(*args[1]);
      float stroke_width = 1.0f;

      static Roo::MapSchema line_opts(
        {},
        {{std::get<std::string>(MapKey::COLOR->value), &HostType::COLOR},
         {std::get<std::string>(MapKey::STROKE_WIDTH->value), &Roo::Type::NUMBER}});

      if (args.size() == 3 && Roo::is_truthy(*args[2]))
      {
        if (dict_contains(args[2], std::get<std::string>(MapKey::COLOR->value)) ||
            dict_contains(args[2], std::get<std::string>(MapKey::STROKE_WIDTH->value)))
        {
          auto opts = line_opts.bind(ctx, *args[2]);
          auto color = opts.optional_obj<Color>(std::get<std::string>(MapKey::COLOR->value));
          if (color)
          {
            SDL_SetRenderDrawColor(rc.renderer, color->r, color->g, color->b, color->a);
          }
          stroke_width = opts.f32(std::get<std::string>(MapKey::STROKE_WIDTH->value), 1.0f);
        }
        else
        {
          Roo::sptr_val color_value = args[2];
          if (!HostType::COLOR.is_type_of(*color_value))
          {
            auto coercion = HostType::COLOR.coerce(ctx, color_value);
            if (!coercion.success)
            {
              throw Roo::TypeError("line!: third argument must be a color or options map.");
            }
            color_value = coercion.result;
          }
          const Color& color = Roo::obj<Color>(*color_value);
          SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
        }
      }

      if (stroke_width <= 0.0f) return Roo::Constant::NIL;

      SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
      draw_stroked_segment(rc.renderer, from, to, stroke_width);
      SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);

      return Roo::Constant::NIL;
    }

    /* DrawCircleBang - circle! */
    FUNC_IMPL(DrawCircleBang,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&DrawCircleBang::exec_draw_circle))));

    EXEC_BODY(DrawCircleBang, exec_draw_circle)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      static Roo::MapSchema circle_schema(
        {{"x", &Roo::Type::NUMBER}, {"y", &Roo::Type::NUMBER}, {"r", &Roo::Type::NUMBER}});
      static Roo::MapSchema opts_schema(
        {},
        {{"color", &HostType::COLOR}, {"fill", &Roo::Type::BOOL}});

      auto circle = circle_schema.bind(ctx, *args[0]);
      auto opts = opts_schema.bind(ctx, *args[1]);

      const int cx = circle.i32("x");
      const int cy = circle.i32("y");
      const int radius = circle.i32("r");
      if (radius < 0) return Roo::Constant::NIL;

      auto color_opt = opts.val("color");
      auto fill_opt = opts.val("fill");

      if (Roo::is_truthy(*color_opt))
      {
        const Color& color = Roo::obj<Color>(*color_opt);
        SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
      }

      SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
      if (Roo::is_truthy(*fill_opt))
      {
        draw_filled_circle(rc.renderer, cx, cy, radius);
      }
      else
      {
        draw_circle_outline(rc.renderer, cx, cy, radius);
      }
      SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);

      return Roo::Constant::NIL;
    }

    /* DrawEllipseBang - ellipse! */
    FUNC_IMPL(DrawEllipseBang,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::MAP)),
                   EXEC_DISPATCH(&DrawEllipseBang::exec_draw_ellipse))));

    EXEC_BODY(DrawEllipseBang, exec_draw_ellipse)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      static Roo::MapSchema ellipse_schema({{"x", &Roo::Type::NUMBER},
                                            {"y", &Roo::Type::NUMBER},
                                            {"rx", &Roo::Type::NUMBER},
                                            {"ry", &Roo::Type::NUMBER}});
      static Roo::MapSchema opts_schema(
        {},
        {{"color", &HostType::COLOR}, {"fill", &Roo::Type::BOOL}});

      auto ellipse = ellipse_schema.bind(ctx, *args[0]);
      auto opts = opts_schema.bind(ctx, *args[1]);

      const int cx = ellipse.i32("x");
      const int cy = ellipse.i32("y");
      const int rx = ellipse.i32("rx");
      const int ry = ellipse.i32("ry");
      if (rx < 0 || ry < 0) return Roo::Constant::NIL;

      auto color_opt = opts.val("color");
      auto fill_opt = opts.val("fill");

      if (Roo::is_truthy(*color_opt))
      {
        const Color& color = Roo::obj<Color>(*color_opt);
        SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
      }

      SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
      if (Roo::is_truthy(*fill_opt))
      {
        draw_filled_ellipse(rc.renderer, cx, cy, rx, ry);
      }
      else
      {
        draw_ellipse_outline(rc.renderer, cx, cy, rx, ry);
      }
      SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);

      return Roo::Constant::NIL;
    }

    /* DrawPolygonbang - polygon! */
    FUNC_IMPL(DrawPolygonBang,
              MULTI_SIG((FN_ARGS((&HostType::VECTOR_OF_POINT)),
                         EXEC_DISPATCH(&DrawPolygonBang::exec_polygon)),
                        (FN_ARGS((&HostType::VECTOR_OF_POINT), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&DrawPolygonBang::exec_polygon_with_opts))));

    Roo::MapSchema polygon_opts(
      {},
      {{std::get<std::string>(MapKey::CLOSE->value), &Roo::Type::BOOL},
       {std::get<std::string>(MapKey::ROTATION->value), &Roo::Type::NUMBER},
       {std::get<std::string>(MapKey::OFFSET->value), &HostType::POINT},
       {std::get<std::string>(MapKey::COLOR->value), &HostType::COLOR},
       {std::get<std::string>(MapKey::FILL->value), &Roo::Type::BOOL},
       {std::get<std::string>(MapKey::FILL_STYLE->value), &Roo::Type::MAP},
       {std::get<std::string>(MapKey::STROKE_WIDTH->value), &Roo::Type::NUMBER},
       {std::get<std::string>(MapKey::SCALE->value), &Roo::Type::NUMBER}});

    EXEC_BODY(DrawPolygonBang, exec_polygon)
    {
      Roo::sptr_val_v opt_args = args;
      opt_args.push_back(Roo::map({}));
      return this->exec_polygon_with_opts(ctx, opt_args);
    }

    EXEC_BODY(DrawPolygonBang, exec_polygon_with_opts)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      auto opts = polygon_opts.bind(ctx, *args.back());

      Roo::sptr_val& polygon = args.front();
      Roo::sptr_val_v points = Roo::get_children(*polygon);

      bool close_shape = opts.boolean(std::get<std::string>(MapKey::CLOSE->value), false);

      float rotation = opts.f32(std::get<std::string>(MapKey::ROTATION->value), 0.0f);
      float scale = opts.f32(std::get<std::string>(MapKey::SCALE->value), 1.0f);
      std::optional<Color> color =
        opts.optional_obj<Color>(std::get<std::string>(MapKey::COLOR->value));
      std::optional<PolygonFillStyle> fill_style = std::nullopt;
      if (auto fill_style_value = opts.val(std::get<std::string>(MapKey::FILL_STYLE->value));
          fill_style_value && fill_style_value->type != Roo::Value::Type::NIL)
      {
        fill_style = parse_polygon_fill_style(ctx, fill_style_value);
      }
      bool fill_shape = opts.boolean(std::get<std::string>(MapKey::FILL->value), false);
      const bool explicit_stroke_width =
        opts.contains(std::get<std::string>(MapKey::STROKE_WIDTH->value));
      float stroke_width = opts.f32(std::get<std::string>(MapKey::STROKE_WIDTH->value),
                                    fill_shape ? 0.0f : 1.0f);

      const Point& offset =
        opts.obj<Point>(std::get<std::string>(MapKey::OFFSET->value), POINT__ZERO_ZERO);

      if (color)
      {
        SDL_SetRenderDrawColor(rc.renderer, color->r, color->g, color->b, color->a);
      }

      if (fill_style && color && fill_shape && !explicit_stroke_width)
      {
        throw Roo::TypeError(
          "polygon!: use either top-level :color or :fill-style for a filled polygon.");
      }

      std::vector<Point> pts;
      if (points.size() > 0)
      {
        pts.reserve(points.size());
        for (auto& poly_pt : points)
        {
          pts.push_back((Roo::obj<Point>(*poly_pt) * scale)
                          .rotate(POINT__ZERO_ZERO, rotation)
                          .plus(offset.x, offset.y));
        }

        if (fill_shape)
        {
          std::vector<float> intersections;
          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
          if (fill_style && fill_style->type == PolygonFillStyle::Type::VERTEX_COLORS)
          {
            if (fill_style->colors.size() != pts.size())
            {
              throw Roo::TypeError(
                "polygon!: :fill-style :vertex-colors :colors count must match points.");
            }
            draw_filled_vertex_colored_polygon(rc.renderer, pts, fill_style->colors);
          }
          else
          {
            if (fill_style && fill_style->type == PolygonFillStyle::Type::SOLID &&
                fill_style->color)
            {
              const Color& fill_color = *fill_style->color;
              SDL_SetRenderDrawColor(rc.renderer,
                                     fill_color.r,
                                     fill_color.g,
                                     fill_color.b,
                                     fill_color.a);
            }
            draw_filled_polygon(rc.renderer, pts, intersections);
          }
          if (color)
          {
            SDL_SetRenderDrawColor(rc.renderer, color->r, color->g, color->b, color->a);
          }
          draw_stroked_polyline(rc.renderer, pts, true, stroke_width);
          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);
        }
        else
        {
          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
          draw_stroked_polyline(rc.renderer, pts, close_shape, stroke_width);
          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);
        }
      }

      return Roo::Constant::NIL;
    }

    /* DrawRectBang - rect! */
    FUNC_IMPL(DrawRectBang,
              MULTI_SIG((FN_ARGS((&HostType::RECT), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&DrawRectBang::exec_draw_rect)),
                        (FN_ARGS((&HostType::POINT), (&HostType::POINT), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&DrawRectBang::exec_draw_rect_from_points))));

    EXEC_BODY(DrawRectBang, exec_draw_rect)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const Rect& hrect = Roo::obj<Rect>(*args[0]);
      const Point top_left = hrect.top_left();
      const Point bottom_right = hrect.bottom_right();

      static Roo::MapSchema opts_schema(
        {},
        {{"color", &HostType::COLOR}, {"fill", &Roo::Type::BOOL}});

      auto opts = opts_schema.bind(ctx, *args[1]);

      auto color_opt = opts.val("color");
      auto fill_opt = opts.val("fill");

      if (Roo::is_truthy(*color_opt))
      {
        const Color& color = Roo::obj<Color>(*color_opt);
        SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
      }

      const Point wh = bottom_right - top_left;

      SDL_Rect rect = {top_left.round_x(), top_left.round_y(), wh.round_x(), wh.round_y()};

      if (Roo::is_truthy(*fill_opt))
      {
        SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(rc.renderer, &rect);
        SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);
      }
      else
      {
        if (rect.w > 0 && rect.h > 0)
        {
          SDL_Rect top = {rect.x, rect.y, rect.w, 1};
          SDL_Rect bottom = {rect.x, rect.y + rect.h - 1, rect.w, 1};
          SDL_Rect left = {rect.x, rect.y, 1, rect.h};
          SDL_Rect right = {rect.x + rect.w - 1, rect.y, 1, rect.h};

          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
          SDL_RenderFillRect(rc.renderer, &top);
          SDL_RenderFillRect(rc.renderer, &bottom);
          SDL_RenderFillRect(rc.renderer, &left);
          SDL_RenderFillRect(rc.renderer, &right);
          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);
        }
      }

      return Roo::Constant::NIL;
    }

    EXEC_BODY(DrawRectBang, exec_draw_rect_from_points)
    {
      const Point& top_left = Roo::obj<Point>(*args[0]);
      const Point& bottom_right = Roo::obj<Point>(*args[1]);

      Roo::sptr_val_v n_args{
        RectAdapter::make_unique(Rect{static_cast<int>(top_left.x),
                                      static_cast<int>(top_left.y),
                                      static_cast<int>(bottom_right.x - top_left.x),
                                      static_cast<int>(bottom_right.y - top_left.y)}),
        args[2]};

      return exec_draw_rect(ctx, n_args);
    }

    /* RenderTextBang - text! */
    FUNC_IMPL(
      RenderTextBang,
      MULTI_SIG((FN_ARGS((&Roo::Type::STRING), (&HostType::POINT)),
                 EXEC_DISPATCH(&RenderTextBang::exec_text_no_opts)),
                (FN_ARGS((&Roo::Type::STRING), (&HostType::POINT), (&Roo::Type::MAP)),
                 EXEC_DISPATCH(&RenderTextBang::exec_text))));

    static Roo::MapSchema text_opts_schema({},
                                           {{"font", &Roo::Type::KEYWORD},
                                            {"color", &HostType::COLOR},
                                            {"scale", &Roo::Type::ANY},
                                            {"font-styles", &Roo::Type::ANY},
                                            {"shadow", &Roo::Type::ANY},
                                            {"marked-style", &Roo::Type::ANY}});

    static Text::Scale parse_text_scale(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return Text::Scale(1);
      if (value->type == Roo::Value::Type::NUMBER) return Text::Scale(value->f32());
      if (value->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("Text scale must be a number or [x y] vector");
      }

      auto children = Roo::get_children(*value);
      if (children.size() != 2 || children[0]->type != Roo::Value::Type::NUMBER ||
          children[1]->type != Roo::Value::Type::NUMBER)
      {
        throw Roo::TypeError("Text scale vector must be [x y] numbers");
      }
      return Text::Scale(children[0]->f32(), children[1]->f32());
    }

    static std::vector<Text::FontStyle> parse_font_styles(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return {};

      auto parse_one = [](const Roo::sptr_val& style_value)
      {
        if (!style_value || style_value->type != Roo::Value::Type::KEYWORD)
        {
          throw Roo::TypeError("Text font style must be a keyword");
        }

        if (style_value->str() == "underline") return Text::FontStyle::UNDERLINE;
        throw Roo::TypeError("Unknown text font style: " + style_value->to_string());
      };

      if (value->type == Roo::Value::Type::KEYWORD) return {parse_one(value)};
      if (value->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("Text font styles must be a keyword or vector");
      }

      std::vector<Text::FontStyle> out;
      for (auto& child : Roo::get_children(*value))
      {
        out.push_back(parse_one(child));
      }
      return out;
    }

    static std::vector<Text::Shadow> parse_shadows(Roo::Context& ctx,
                                                   const Roo::sptr_val& shadow_val)
    {
      std::vector<Text::Shadow> shadows;
      if (!shadow_val || shadow_val->type == Roo::Value::Type::NIL) return shadows;

      static Roo::MapSchema shadow_schema(
        {{"offset", &HostType::POINT}, {"color", &HostType::COLOR}},
        {});

      auto parse_one = [&](const Roo::sptr_val& s)
      {
        auto sh = shadow_schema.bind(ctx, *s);
        return Text::Shadow(sh.obj<Point>("offset"), sh.obj<Color>("color"));
      };

      if (shadow_val->type == Roo::Value::Type::VECTOR)
      {
        for (auto& s : Roo::get_children(*shadow_val))
          shadows.push_back(parse_one(s));
      }
      else if (shadow_val->type == Roo::Value::Type::MAP)
      {
        shadows.push_back(parse_one(shadow_val));
      }

      return shadows;
    }

    static std::optional<char> parse_inline_marker(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;
      if (value->type == Roo::Value::Type::CHAR) return static_cast<char>(value->ch());
      if (value->type == Roo::Value::Type::STRING ||
          value->type == Roo::Value::Type::KEYWORD ||
          value->type == Roo::Value::Type::SYMBOL)
      {
        std::string raw = value->str();
        if (raw.size() == 1) return raw[0];
      }
      return std::nullopt;
    }

    static std::optional<Text::InlineTextStyleSpec> parse_marked_style(
      Roo::Context& ctx,
      const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

      static Roo::MapSchema inline_schema({},
                                          {{"enabled", &Roo::Type::BOOL},
                                           {"marker", &Roo::Type::ANY},
                                           {"font", &Roo::Type::KEYWORD},
                                           {"color", &HostType::COLOR},
                                           {"scale", &Roo::Type::ANY},
                                           {"font-styles", &Roo::Type::ANY},
                                           {"shadow", &Roo::Type::ANY}});

      auto inline_source = value;
      if (Roo::Dict::contains_key(*value, "color"))
      {
        auto color_value = Roo::Dict::get_property(*value, "color");
        if (color_value && color_value->type == Roo::Value::Type::KEYWORD &&
            color_value->str() == "none")
        {
          inline_source = Roo::Dict::shallow_copy(value);
          Roo::Dict::set_property(inline_source, Roo::keyword("color"), Roo::Constant::NIL);
        }
      }

      auto opts = inline_schema.bind(ctx, *inline_source);
      Text::InlineTextStyleSpec spec;
      spec.enabled = opts.contains("enabled") ? opts.boolean("enabled") : true;
      if (auto marker = parse_inline_marker(opts.val("marker")); marker.has_value())
      {
        spec.marker = *marker;
      }
      if (opts.contains("font")) spec.font_key = opts.str("font");
      if (auto color_value = opts.val("color");
          color_value && color_value->type == Roo::Value::Type::KEYWORD &&
          color_value->str() == "none")
      {
        spec.use_font_color = true;
      }
      else if (auto color_value = opts.val("color");
               color_value && color_value->type != Roo::Value::Type::NIL)
      {
        spec.color = Roo::obj<Color>(*color_value);
      }
      if (opts.contains("scale")) spec.scale = parse_text_scale(opts.val("scale"));
      if (opts.contains("font-styles"))
      {
        spec.font_styles = parse_font_styles(opts.val("font-styles"));
      }
      if (opts.contains("shadow"))
      {
        spec.shadows = parse_shadows(ctx, opts.val("shadow"));
      }
      return spec;
    }

    static Roo::sptr_val make_rect_map(int x, int y, int w, int h)
    {
      auto map = Roo::map({});
      auto vx = Roo::number(x);
      auto vy = Roo::number(y);
      auto vw = Roo::number(w);
      auto vh = Roo::number(h);
      Roo::Dict::set_property(map, Roo::keyword("x"), vx);
      Roo::Dict::set_property(map, Roo::keyword("y"), vy);
      Roo::Dict::set_property(map, Roo::keyword("w"), vw);
      Roo::Dict::set_property(map, Roo::keyword("h"), vh);
      return map;
    }

    EXEC_BODY(RenderTextBang, exec_text_no_opts)
    {
      Roo::sptr_val_v full_args = args;
      full_args.push_back(Roo::map({}));
      return this->exec_text(ctx, full_args);
    }

    EXEC_BODY(RenderTextBang, exec_text)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const std::string& text = args[0]->str();
      const Point& pos = Roo::obj<Point>(*args[1]);
      auto opts = text_opts_schema.bind(ctx, *args[2]);

      std::string font_key = "font/console";
      if (auto fv = opts.val("font"); fv && fv->type == Roo::Value::Type::KEYWORD)
        font_key = fv->str();

      Text::Scale scale(1);
      if (auto sv = opts.val("scale"); sv && sv->type != Roo::Value::Type::NIL)
        scale = parse_text_scale(sv);

      std::optional<Color> color;
      if (auto cv = opts.val("color"); cv && cv->type != Roo::Value::Type::NIL)
      {
        color = Roo::obj<Color>(*cv);
      }

      std::vector<Text::FontStyle> font_styles;
      if (auto fsv = opts.val("font-styles"); fsv && fsv->type != Roo::Value::Type::NIL)
      {
        font_styles = parse_font_styles(fsv);
      }

      std::vector<Text::Shadow> shadows;
      if (auto sv = opts.val("shadow"); sv && sv->type != Roo::Value::Type::NIL)
      {
        shadows = parse_shadows(ctx, sv);
      }

      auto inline_style = parse_marked_style(ctx, opts.val("marked-style"));

      auto text_op = Text::make_text_render_op(rc,
                                               font_key,
                                               scale,
                                               color,
                                               font_styles,
                                               shadows,
                                               inline_style);
      if (!text_op) return Roo::Constant::NIL;

      Text::render_text(rc, *text_op, text, pos.round_x(), pos.round_y());

      SDL_Rect size = Text::calculate_rendered_size(rc, *text_op, text);
      return make_rect_map(pos.round_x(), pos.round_y(), size.w, size.h);
    }

    /* TextSize - text-size */
    FUNC_IMPL(TextSize,
              MULTI_SIG((FN_ARGS((&Roo::Type::STRING)),
                         EXEC_DISPATCH(&TextSize::exec_size_no_opts)),
                        (FN_ARGS((&Roo::Type::STRING), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&TextSize::exec_size))));

    static Roo::MapSchema text_size_opts_schema({},
                                                {{"font", &Roo::Type::KEYWORD},
                                                 {"scale", &Roo::Type::ANY},
                                                 {"font-styles", &Roo::Type::ANY},
                                                 {"shadow", &Roo::Type::ANY},
                                                 {"marked-style", &Roo::Type::ANY}});

    EXEC_BODY(TextSize, exec_size_no_opts)
    {
      Roo::sptr_val_v full_args = args;
      full_args.push_back(Roo::map({}));
      return this->exec_size(ctx, full_args);
    }

    EXEC_BODY(TextSize, exec_size)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const std::string& text = args[0]->str();
      auto opts = text_size_opts_schema.bind(ctx, *args[1]);

      std::string font_key = "font/console";
      if (auto fv = opts.val("font"); fv && fv->type == Roo::Value::Type::KEYWORD)
        font_key = fv->str();

      Text::Scale scale(1);
      if (auto sv = opts.val("scale"); sv && sv->type != Roo::Value::Type::NIL)
        scale = parse_text_scale(sv);

      std::vector<Text::FontStyle> font_styles;
      if (auto fsv = opts.val("font-styles"); fsv && fsv->type != Roo::Value::Type::NIL)
      {
        font_styles = parse_font_styles(fsv);
      }

      std::vector<Text::Shadow> shadows;
      if (auto sv = opts.val("shadow"); sv && sv->type != Roo::Value::Type::NIL)
      {
        shadows = parse_shadows(ctx, sv);
      }

      auto inline_style = parse_marked_style(ctx, opts.val("marked-style"));

      auto text_op = Text::make_text_render_op(rc,
                                               font_key,
                                               scale,
                                               std::nullopt,
                                               font_styles,
                                               shadows,
                                               inline_style);
      if (!text_op) return Roo::Constant::NIL;

      SDL_Rect size = Text::calculate_rendered_size(rc, *text_op, text);

      auto map = Roo::map({});
      auto vw = Roo::number(size.w);
      auto vh = Roo::number(size.h);
      Roo::Dict::set_property(map, Roo::keyword("w"), vw);
      Roo::Dict::set_property(map, Roo::keyword("h"), vh);
      return map;
    }

    /* UseColorBang */
    FUNC_IMPL(UseColorBang,
              MULTI_SIG((FN_ARGS((&HostType::COLOR)),
                         EXEC_DISPATCH(&UseColorBang::exec_use_color)),
                        (FN_ARGS((&Roo::Type::NUMBER),
                                 (&Roo::Type::NUMBER),
                                 (&Roo::Type::NUMBER),
                                 (&Roo::Type::NUMBER)),
                         EXEC_DISPATCH(&UseColorBang::exec_use_color_num))));

    EXEC_BODY(UseColorBang, exec_use_color)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const Color& color = Roo::obj<Color>(*args[0]);
      SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);

      return Roo::Constant::NIL;
    }

    EXEC_BODY(UseColorBang, exec_use_color_num)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const int r = args[0]->num().get_int();
      const int g = args[1]->num().get_int();
      const int b = args[2]->num().get_int();
      const int a = args[3]->num().get_int();

      SDL_SetRenderDrawColor(rc.renderer, r, g, b, a);

      return Roo::Constant::NIL;
    }

    /* WithClipRectForm - with-clip-rect */
    SPECIAL_FORM_IMPL(WithClipRectForm,
                      SIG((FN_ARGS((&HostType::RECT),
                                   (Roo::VARARG, &Roo::Type::ANY, NO_EVAL)),
                           EXEC_DISPATCH(&WithClipRectForm::execnode_with_clip_rect))))

    SFORM_LOWER_IMPL(WithClipRectForm)
    {
      auto& elements = ast_node->get_children();
      if (elements.size() < 2)
      {
        throw Roo::RooException("Invalid with-clip-rect form: " + ast_node->to_string());
      }

      Roo::uptr_exec_node_v exec_nodes;
      exec_nodes.reserve(elements.size() - 1);
      for (size_t i = 1; i < elements.size(); i++)
      {
        exec_nodes.push_back(Roo::lower_expr(ctx, elements[i]));
      }

      return std::make_unique<Roo::ExecNode>(
        Roo::SpecialFormNode(this, Roo::sptr_val_v{}, std::move(exec_nodes)));
    }

    EXECNODE_BODY(WithClipRectForm, execnode_with_clip_rect)
    {
      RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      if (snode.exec_nodes.empty())
      {
        throw Roo::InvocationException("Invalid with-clip-rect execution node.");
      }

      Roo::sptr_val clip_value = Roo::exec(ctx, *snode.exec_nodes.front());
      if (!HostType::RECT.is_type_of(*clip_value))
      {
        auto coercion = HostType::RECT.coerce(ctx, clip_value);
        if (!coercion.success)
        {
          throw Roo::TypeError("with-clip-rect: clip rect must be a Rect or rect map.");
        }
        clip_value = coercion.result;
      }

      const Rect requested_clip = Roo::obj<Rect>(*clip_value);
      std::optional<Rect> previous_clip = rc.current_clip_rect;
      rc.set_clip_rect(intersect_clip_rect(previous_clip, requested_clip));

      Roo::sptr_val result = Roo::Constant::NIL;
      try
      {
        for (size_t i = 1; i < snode.exec_nodes.size(); i++)
        {
          result = Roo::exec(ctx, *snode.exec_nodes[i]);
        }
      }
      catch (...)
      {
        rc.set_clip_rect(previous_clip);
        throw;
      }

      rc.set_clip_rect(previous_clip);
      return result;
    }

  } // namespace Function

  RenderNamespace::RenderNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__RENDER))
  {
    values.emplace(FN__DRAW_IMAGE_BANG, Function::DrawImageBang::make());
    values.emplace(FN__DRAW_CIRCLE_BANG, Function::DrawCircleBang::make());
    values.emplace(FN__DRAW_ELLIPSE_BANG, Function::DrawEllipseBang::make());
    values.emplace(FN__DRAW_LINE_BANG, Function::DrawLineBang::make());
    values.emplace(FN__DRAW_POLYGON_BANG, Function::DrawPolygonBang::make());
    values.emplace(FN__DRAW_RECT_BANG, Function::DrawRectBang::make());
    values.emplace(FN__RENDER_TEXT_BANG, Function::RenderTextBang::make());
    values.emplace(FN__TEXT_SIZE, Function::TextSize::make());
    values.emplace(FN__USE_COLOR_BANG, Function::UseColorBang::make());
    values.emplace(FN__WITH_CLIP_RECT, Function::WithClipRectForm::make());
  }
} // namespace Pixils::Script
