#ifndef PIXILS__UI__VIEW_LAYOUT_H
#define PIXILS__UI__VIEW_LAYOUT_H

#include <pixils/geom.h>
#include <pixils/ui/style.h>
#include <pixils/ui/theme.h>

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
   * Resolve the theme that applies to a view without requiring a full layout
   * pass. This is used when a newly pushed mode needs to inherit the current
   * mode's theme before the current mode has rendered.
   */
  Theme resolve_effective_theme(const std::shared_ptr<Pixils::Runtime::View>& view,
                                Lisple::Runtime& runtime,
                                const Theme* inherited_theme = nullptr);

  /**
   * Recursively assign bounds to a view tree starting from the given root
   * bounds. Child placement uses the same flow/absolute rules as
   * layout_children, but writes the results onto each live view so later passes
   * can consume them without recomputing layout.
   */
  void layout_view_tree(const std::shared_ptr<Pixils::Runtime::View>& view,
                        const Rect& bounds,
                        Lisple::Runtime& runtime,
                        const Lisple::sptr_val& hook_ctx);

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
    const Lisple::sptr_val& hook_ctx,
    const Style::Layout& layout = {},
    const Style* inherited_style = nullptr,
    const Theme* inherited_theme = nullptr);

} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_LAYOUT_H */
