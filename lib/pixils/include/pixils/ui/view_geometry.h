#ifndef PIXILS__UI__VIEW_GEOMETRY_H
#define PIXILS__UI__VIEW_GEOMETRY_H

#include <pixils/geom.h>
#include <pixils/ui/style.h>

#include <algorithm>

namespace Pixils::UI
{
  inline int style_scale_factor(const Style& style)
  {
    return std::max(1, style.scale.value_or(1));
  }

  inline Rect scaled_external_bounds(const Rect& logical_bounds, const Style& style)
  {
    int scale = style_scale_factor(style);
    return {logical_bounds.x,
            logical_bounds.y,
            logical_bounds.w * scale,
            logical_bounds.h * scale};
  }
} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_GEOMETRY_H */
