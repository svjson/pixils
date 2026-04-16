
#ifndef PIXILS__RUNTIME__RENDER_H
#define PIXILS__RUNTIME__RENDER_H

#include "pixils/runtime/session.h"

namespace Pixils
{
  struct Rect;
}

namespace Pixils::Runtime
{
  struct ModeContext;
  struct Session;

  std::vector<Rect> layout_children(const std::vector<ChildSlot>& slots,
                                    const Rect& parent,
                                    LayoutDirection direction = LayoutDirection::COLUMN);

  /**
   * Render a mode context into the given bounds. Applies style (background fill,
   * padding inset), sets a viewport, invokes the render hook, then lays out and
   * recurses into any child contexts. For stack-level modes the caller passes the
   * full buffer rect; for layout children the caller passes the computed slot rect.
   */
  void render_mode_context(Session& session, ModeContext& ctx, const Rect& bounds);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__RENDER_H */
