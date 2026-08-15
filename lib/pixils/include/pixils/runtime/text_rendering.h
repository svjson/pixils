#ifndef PIXILS__RUNTIME__TEXT_RENDERING_H
#define PIXILS__RUNTIME__TEXT_RENDERING_H

#include <pixils/text.h>

#include <optional>
#include <roo/runtime/value.h>

namespace Roo
{
  class Context;
}

namespace Pixils
{
  struct RenderContext;
}

namespace Pixils::Runtime
{
  /**
   * @brief Resolves Roo text options into a native text render operation.
   *
   * @param ctx Roo execution context used for host-value coercion.
   * @param render_context Active Pixils rendering context.
   * @param options Roo text option map.
   * @return A prepared operation, or `std::nullopt` when its font cannot be resolved.
   */
  std::optional<Text::TextRenderOp> resolve_text_render_op(Roo::Context& ctx,
                                                           RenderContext& render_context,
                                                           const Roo::sptr_val& options);
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__TEXT_RENDERING_H */
