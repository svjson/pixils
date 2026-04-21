
#ifndef PIXILS__RUNTIME__RENDER_H
#define PIXILS__RUNTIME__RENDER_H

#include "pixils/runtime/session.h"

namespace Pixils
{
  struct Rect;
}

namespace Pixils::Runtime
{
  struct View;
  struct Session;

  /**
   * Compute absolute layout rects for a set of child views within a parent rect.
   * Direction controls the main axis (COLUMN = vertical, ROW = horizontal).
   * Each child's resolved style supplies its fixed size on the main axis; children
   * without a fixed size share the remaining space equally. Absolute-positioned
   * children (position == ABSOLUTE) are excluded from flow and returned as zero
   * rects - their placement is handled separately in render_view.
   */
  std::vector<Rect> layout_children(const std::vector<std::shared_ptr<View>>& children,
                                    const Rect& parent,
                                    UI::LayoutDirection direction = UI::LayoutDirection::COLUMN);

  /**
   * Render a view into the given bounds. Applies style (background fill,
   * padding inset), sets a viewport, invokes the render hook, then lays out and
   * recurses into any child views. Layout direction and child sizing are read
   * from each child's resolved style. For stack-level modes the caller passes the
   * full buffer rect; for layout children the caller passes the computed slot rect.
   */
  void render_view(Session& session, const std::shared_ptr<View>& view, const Rect& bounds);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__RENDER_H */
