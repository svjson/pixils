#ifndef PIXILS__UI__VIEW_LAYOUT_H
#define PIXILS__UI__VIEW_LAYOUT_H

#include <pixils/geom.h>
#include <pixils/ui/style.h>

#include <lisple/runtime/value.h>
#include <memory>
#include <vector>

namespace Lisple
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct View;
} // namespace Pixils::Runtime

namespace Pixils::UI
{
  /**
   * Compute absolute layout rects for a set of child views within a parent rect.
   * Direction controls the main axis (COLUMN = vertical, ROW = horizontal).
   * Each child's resolved style supplies its fixed size on the main axis; children
   * without a fixed size share the remaining space equally. Absolute-positioned
   * children (position == ABSOLUTE) are excluded from flow and returned as zero
   * rects - their placement is handled separately by view rendering.
   */
  std::vector<Rect> layout_children(
    const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
    const Rect& parent,
    Lisple::Runtime& runtime,
    const Lisple::sptr_rtval& hook_ctx,
    LayoutDirection direction = LayoutDirection::COLUMN);

} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_LAYOUT_H */
