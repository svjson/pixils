#include <pixils/image_trace.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <unordered_map>

namespace Pixils::ImageTrace
{
  namespace
  {
    struct GridPoint
    {
      int x = 0;
      int y = 0;

      bool operator==(const GridPoint& other) const { return x == other.x && y == other.y; }
    };

    struct GridPointHash
    {
      std::size_t operator()(const GridPoint& point) const
      {
        return (static_cast<std::size_t>(static_cast<uint32_t>(point.x)) << 32) ^
               static_cast<std::size_t>(static_cast<uint32_t>(point.y));
      }
    };

    struct Edge
    {
      GridPoint from;
      GridPoint to;
      bool used = false;
    };

    struct Vec2
    {
      float x = 0.0f;
      float y = 0.0f;
    };

    int sign(int value)
    {
      if (value < 0) return -1;
      if (value > 0) return 1;
      return 0;
    }

    int direction_index(const GridPoint& from, const GridPoint& to)
    {
      int dx = sign(to.x - from.x);
      int dy = sign(to.y - from.y);
      if (dx > 0) return 0;
      if (dy > 0) return 1;
      if (dx < 0) return 2;
      return 3;
    }

    int turn_priority(int incoming_dir, int outgoing_dir)
    {
      int turn = (outgoing_dir - incoming_dir + 4) % 4;
      if (turn == 1) return 0;
      if (turn == 0) return 1;
      if (turn == 3) return 2;
      return 3;
    }

    bool opaque_at(const std::vector<uint8_t>& alpha,
                   int width,
                   int height,
                   uint8_t threshold,
                   int x,
                   int y)
    {
      if (x < 0 || y < 0 || x >= width || y >= height) return false;
      return alpha[static_cast<std::size_t>(y * width + x)] >= threshold;
    }

    bool omitted_straight_edge(const Edge& edge,
                               int width,
                               int height,
                               const StraightEdgeMask& mask)
    {
      if (mask.north && edge.from.y == 0 && edge.to.y == 0) return true;
      if (mask.east && edge.from.x == width && edge.to.x == width) return true;
      if (mask.south && edge.from.y == height && edge.to.y == height) return true;
      if (mask.west && edge.from.x == 0 && edge.to.x == 0) return true;
      return false;
    }

    std::vector<Edge> build_edges(const std::vector<uint8_t>& alpha,
                                  int width,
                                  int height,
                                  const TraceOptions& options)
    {
      auto push_edge = [&](std::vector<Edge>& edges, Edge edge)
      {
        if (!omitted_straight_edge(edge, width, height, options.omit_straight_edges))
        {
          edges.push_back(edge);
        }
      };

      std::vector<Edge> edges;

      for (int y = 0; y < height; y++)
      {
        for (int x = 0; x < width; x++)
        {
          if (!opaque_at(alpha, width, height, options.alpha_threshold, x, y)) continue;

          if (!opaque_at(alpha, width, height, options.alpha_threshold, x, y - 1))
          {
            push_edge(edges, {GridPoint{x, y}, GridPoint{x + 1, y}});
          }
          if (!opaque_at(alpha, width, height, options.alpha_threshold, x + 1, y))
          {
            push_edge(edges, {GridPoint{x + 1, y}, GridPoint{x + 1, y + 1}});
          }
          if (!opaque_at(alpha, width, height, options.alpha_threshold, x, y + 1))
          {
            push_edge(edges, {GridPoint{x + 1, y + 1}, GridPoint{x, y + 1}});
          }
          if (!opaque_at(alpha, width, height, options.alpha_threshold, x - 1, y))
          {
            push_edge(edges, {GridPoint{x, y + 1}, GridPoint{x, y}});
          }
        }
      }

      return edges;
    }

    std::size_t choose_next_edge(const std::vector<Edge>& edges,
                                 const std::vector<std::size_t>& candidates,
                                 int incoming_dir)
    {
      std::size_t best = candidates.front();
      int best_priority =
        turn_priority(incoming_dir, direction_index(edges[best].from, edges[best].to));

      for (std::size_t candidate : candidates)
      {
        if (edges[candidate].used) continue;
        int priority =
          turn_priority(incoming_dir,
                        direction_index(edges[candidate].from, edges[candidate].to));
        if (priority < best_priority)
        {
          best = candidate;
          best_priority = priority;
        }
      }

      return best;
    }

    bool is_collinear(const GridPoint& a, const GridPoint& b, const GridPoint& c)
    {
      int abx = b.x - a.x;
      int aby = b.y - a.y;
      int bcx = c.x - b.x;
      int bcy = c.y - b.y;
      return (abx * bcy) - (aby * bcx) == 0;
    }

    std::vector<GridPoint> remove_collinear_points(const std::vector<GridPoint>& ring)
    {
      if (ring.size() < 3) return {};

      std::vector<GridPoint> result;
      result.reserve(ring.size());

      for (std::size_t i = 0; i < ring.size(); i++)
      {
        const GridPoint& prev = ring[(i + ring.size() - 1) % ring.size()];
        const GridPoint& curr = ring[i];
        const GridPoint& next = ring[(i + 1) % ring.size()];
        if (!is_collinear(prev, curr, next)) result.push_back(curr);
      }

      return result.size() >= 3 ? result : std::vector<GridPoint>{};
    }

    std::vector<GridPoint> remove_collinear_points_open(const std::vector<GridPoint>& line)
    {
      if (line.size() < 2) return {};

      std::vector<GridPoint> result;
      result.reserve(line.size());
      result.push_back(line.front());

      for (std::size_t i = 1; i + 1 < line.size(); i++)
      {
        const GridPoint& prev = line[i - 1];
        const GridPoint& curr = line[i];
        const GridPoint& next = line[i + 1];
        if (!is_collinear(prev, curr, next)) result.push_back(curr);
      }

      result.push_back(line.back());
      return result.size() >= 2 ? result : std::vector<GridPoint>{};
    }

    Vec2 side_normal(const GridPoint& from, const GridPoint& to, EdgePlacement placement)
    {
      int dx = sign(to.x - from.x);
      int dy = sign(to.y - from.y);
      Vec2 opaque_side{static_cast<float>(-dy), static_cast<float>(dx)};

      if (placement == EdgePlacement::INNER) return opaque_side;
      return Vec2{-opaque_side.x, -opaque_side.y};
    }

    bool line_intersection(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2, Vec2& out)
    {
      float adx = a2.x - a1.x;
      float ady = a2.y - a1.y;
      float bdx = b2.x - b1.x;
      float bdy = b2.y - b1.y;
      float denom = (adx * bdy) - (ady * bdx);
      if (std::fabs(denom) < 0.000001f) return false;

      float cx = b1.x - a1.x;
      float cy = b1.y - a1.y;
      float t = (cx * bdy - cy * bdx) / denom;
      out = Vec2{a1.x + (t * adx), a1.y + (t * ady)};
      return true;
    }

    Polygon make_polygon(const std::vector<GridPoint>& ring,
                         EdgePlacement placement,
                         bool closed)
    {
      Polygon polygon;
      polygon.reserve(ring.size());

      if (placement == EdgePlacement::BOUNDARY)
      {
        for (const auto& point : ring)
        {
          polygon.emplace_back(static_cast<float>(point.x), static_cast<float>(point.y));
        }
        return polygon;
      }

      constexpr float OFFSET = 0.5f;
      for (std::size_t i = 0; i < ring.size(); i++)
      {
        const bool first_open = !closed && i == 0;
        const bool last_open = !closed && i == ring.size() - 1;
        const GridPoint& prev =
          first_open ? ring[i] : ring[(i + ring.size() - 1) % ring.size()];
        const GridPoint& curr = ring[i];
        const GridPoint& next = last_open ? ring[i] : ring[(i + 1) % ring.size()];

        Vec2 prev_normal = first_open ? side_normal(curr, next, placement)
                                      : side_normal(prev, curr, placement);
        Vec2 next_normal = last_open ? side_normal(prev, curr, placement)
                                     : side_normal(curr, next, placement);

        Vec2 prev_from{static_cast<float>(prev.x) + prev_normal.x * OFFSET,
                       static_cast<float>(prev.y) + prev_normal.y * OFFSET};
        Vec2 prev_to{static_cast<float>(curr.x) + prev_normal.x * OFFSET,
                     static_cast<float>(curr.y) + prev_normal.y * OFFSET};
        Vec2 next_from{static_cast<float>(curr.x) + next_normal.x * OFFSET,
                       static_cast<float>(curr.y) + next_normal.y * OFFSET};
        Vec2 next_to{static_cast<float>(next.x) + next_normal.x * OFFSET,
                     static_cast<float>(next.y) + next_normal.y * OFFSET};

        Vec2 intersection;
        if (line_intersection(prev_from, prev_to, next_from, next_to, intersection))
        {
          polygon.emplace_back(intersection.x, intersection.y);
        }
        else
        {
          polygon.emplace_back(static_cast<float>(curr.x), static_cast<float>(curr.y));
        }
      }

      return polygon;
    }

    bool polygon_less(const Polygon& a, const Polygon& b)
    {
      auto bounds = [](const Polygon& polygon)
      {
        float min_x = polygon.front().x;
        float min_y = polygon.front().y;
        for (const auto& point : polygon)
        {
          min_x = std::min(min_x, point.x);
          min_y = std::min(min_y, point.y);
        }
        return std::pair<float, float>{min_y, min_x};
      };

      return bounds(a) < bounds(b);
    }
  } // namespace

  std::vector<Polygon> trace_alpha_mask(const std::vector<uint8_t>& alpha,
                                        int width,
                                        int height,
                                        const TraceOptions& options)
  {
    if (width <= 0 || height <= 0) return {};
    if (alpha.size() < static_cast<std::size_t>(width * height)) return {};

    std::vector<Edge> edges = build_edges(alpha, width, height, options);

    std::unordered_map<GridPoint, std::vector<std::size_t>, GridPointHash> outgoing;
    for (std::size_t i = 0; i < edges.size(); i++)
    {
      outgoing[edges[i].from].push_back(i);
    }

    std::vector<Polygon> polygons;
    const bool allow_open_paths = options.omit_straight_edges.any();
    std::unordered_map<GridPoint, std::vector<std::size_t>, GridPointHash> incoming;
    for (std::size_t i = 0; i < edges.size(); i++)
    {
      incoming[edges[i].to].push_back(i);
    }

    std::vector<std::size_t> trace_order;
    trace_order.reserve(edges.size());
    if (allow_open_paths)
    {
      for (std::size_t i = 0; i < edges.size(); i++)
      {
        if (incoming.find(edges[i].from) == incoming.end()) trace_order.push_back(i);
      }
    }
    for (std::size_t i = 0; i < edges.size(); i++)
      trace_order.push_back(i);

    for (std::size_t start_index : trace_order)
    {
      std::size_t i = start_index;
      if (edges[i].used) continue;

      std::vector<GridPoint> ring;
      GridPoint start = edges[i].from;
      std::size_t current = i;
      bool closed = false;

      while (!edges[current].used)
      {
        Edge& edge = edges[current];
        edge.used = true;
        ring.push_back(edge.from);

        if (edge.to == start)
        {
          closed = true;
          break;
        }

        auto candidates = outgoing.find(edge.to);
        if (candidates == outgoing.end())
        {
          if (allow_open_paths) ring.push_back(edge.to);
          break;
        }

        std::vector<std::size_t> available;
        for (std::size_t candidate : candidates->second)
        {
          if (!edges[candidate].used) available.push_back(candidate);
        }
        if (available.empty())
        {
          if (allow_open_paths) ring.push_back(edge.to);
          break;
        }

        current = choose_next_edge(edges, available, direction_index(edge.from, edge.to));
      }

      if (!closed && !allow_open_paths) continue;

      std::vector<GridPoint> simplified =
        closed ? remove_collinear_points(ring) : remove_collinear_points_open(ring);
      if (simplified.size() < (closed ? 3u : 2u)) continue;

      Polygon polygon = make_polygon(simplified, options.edge, closed);
      if (polygon.size() >= (closed ? 3u : 2u)) polygons.push_back(std::move(polygon));
    }

    std::sort(polygons.begin(), polygons.end(), polygon_less);
    return polygons;
  }
} // namespace Pixils::ImageTrace
