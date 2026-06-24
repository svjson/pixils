#ifndef PIXILS__IMAGE_TRACE_H
#define PIXILS__IMAGE_TRACE_H

#include <pixils/geom.h>

#include <cstdint>
#include <vector>

namespace Pixils::ImageTrace
{
  enum class EdgePlacement
  {
    OUTER,
    INNER,
    BOUNDARY
  };

  struct StraightEdgeMask
  {
    bool north = false;
    bool east = false;
    bool south = false;
    bool west = false;

    bool any() const { return north || east || south || west; }
  };

  struct TraceOptions
  {
    uint8_t alpha_threshold = 1;
    EdgePlacement edge = EdgePlacement::OUTER;
    StraightEdgeMask omit_straight_edges;
  };

  using Polygon = std::vector<Point>;

  std::vector<Polygon> trace_alpha_mask(const std::vector<uint8_t>& alpha,
                                        int width,
                                        int height,
                                        const TraceOptions& options = {});
} // namespace Pixils::ImageTrace

#endif /* PIXILS__IMAGE_TRACE_H */
