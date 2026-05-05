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
   * Render a view using its already-resolved bounds. Applies style (background
   * fill, border, content inset), sets a viewport, invokes the render hook, and
   * recurses into any child views that were already laid out by the separate
   * layout pass.
   */
  void render_view(Pixils::RenderContext& render_ctx,
                   Lisple::Runtime& runtime,
                   const Lisple::sptr_rtval& render_hook_ctx,
                   const std::shared_ptr<Pixils::Runtime::View>& view);

} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_RENDER_H */
