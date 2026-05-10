
#ifndef __PIXILS__HOOK_CONTEXT_H_
#define __PIXILS__HOOK_CONTEXT_H_

#include <pixils/frame_events.h>
#include <memory>
#include <optional>

namespace Pixils
{
  struct RenderContext;
}

namespace Pixils::Runtime
{
  struct View;
}

namespace Pixils
{
  /**
   * Bundles per-frame input state and render context info into the single
   * context object passed as the second argument to all mode hooks.
   */
  struct HookContext
  {
    FrameEvents* events;
    const RenderContext* render;
    std::shared_ptr<Runtime::View> current_view = nullptr;
    std::optional<int> available_width = std::nullopt;
    std::optional<int> available_height = std::nullopt;
  };

} // namespace Pixils

#endif /* __PIXILS__HOOK_CONTEXT_H_ */
