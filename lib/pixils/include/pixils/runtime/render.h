
#ifndef PIXILS__RUNTIME__RENDER_H
#define PIXILS__RUNTIME__RENDER_H

#include "pixils/runtime/session.h"

namespace Pixils
{
  struct Rect;
}

namespace Pixils::Runtime
{
  struct ChildContext;
  struct Session;

  std::vector<Rect> layout_children(const std::vector<ChildSlot>& slots,
                                    const Rect& parent,
                                    LayoutDirection direction = LayoutDirection::COLUMN);
  void render_child(Session& session, ChildContext& child, const Rect& bounds);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__RENDER_H */
