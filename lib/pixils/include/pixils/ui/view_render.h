#ifndef PIXILS__UI__VIEW_RENDER_H
#define PIXILS__UI__VIEW_RENDER_H

#include <pixils/geom.h>
#include <pixils/ui/style.h>

#include <lisple/form.h>
#include <memory>
#include <vector>

namespace Lisple
{
  class Runtime;
}

namespace Pixils
{
  struct RenderContext;
}

namespace Pixils::Runtime
{
  struct View;
} // namespace Pixils::Runtime

namespace Pixils::UI
{
  /**
   * Render a view into the given bounds. Applies style (background fill,
   * padding inset), sets a viewport, invokes the render hook, then lays out and
   * recurses into any child views. Layout direction and child sizing are read
   * from each child's resolved style. For stack-level modes the caller passes the
   * full buffer rect; for layout children the caller passes the computed slot rect.
   */
  void render_view(Pixils::RenderContext& render_ctx,
                   Lisple::Runtime& runtime,
                   const Lisple::sptr_rtval& render_hook_ctx,
                   const std::shared_ptr<Pixils::Runtime::View>& view,
                   const Rect& bounds);

} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_RENDER_H */
