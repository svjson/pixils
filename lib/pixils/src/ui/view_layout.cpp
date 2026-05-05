#include "pixils/ui/view_layout.h"

#include <pixils/runtime/view.h>

namespace Pixils::UI
{
  std::vector<Rect> layout_children(
    const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
    const Rect& parent,
    LayoutDirection direction)
  {
    bool row = direction == LayoutDirection::ROW;

    int total_fixed = 0;
    int fill_count = 0;
    for (const auto& child : children)
    {
      Style cs = resolve_style(child->mode->style, child->state, child->interaction);
      if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
      const auto& size_opt = row ? cs.width : cs.height;
      if (size_opt)
        total_fixed += row ? cs.total_width() : cs.total_height();
      else
        fill_count++;
    }

    int available = row ? parent.w : parent.h;
    int fill_size = fill_count > 0 ? (available - total_fixed) / fill_count : 0;

    std::vector<Rect> rects;
    rects.reserve(children.size());

    int pos = row ? parent.x : parent.y;
    for (const auto& child : children)
    {
      Style cs = resolve_style(child->mode->style, child->state, child->interaction);
      const Style::Insets margin = cs.margin.value_or(Style::Insets{});
      if (cs.position && *cs.position == PositionMode::ABSOLUTE)
      {
        rects.push_back({0, 0, 0, 0});
        continue;
      }
      const auto& size_opt = row ? cs.width : cs.height;
      int outer_size = size_opt ? (row ? cs.total_width() : cs.total_height()) : fill_size;
      int cross_outer_size = row ? (cs.height ? cs.total_height() : parent.h)
                                 : (cs.width ? cs.total_width() : parent.w);
      if (row)
      {
        rects.push_back({pos + margin.l,
                         parent.y + margin.t,
                         std::max(0, outer_size - margin.l - margin.r),
                         std::max(0, cross_outer_size - margin.t - margin.b)});
      }
      else
      {
        rects.push_back({parent.x + margin.l,
                         pos + margin.t,
                         std::max(0, cross_outer_size - margin.l - margin.r),
                         std::max(0, outer_size - margin.t - margin.b)});
      }
      pos += outer_size;
    }

    return rects;
  }

} // namespace Pixils::UI
