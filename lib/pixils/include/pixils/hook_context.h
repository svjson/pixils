
#ifndef __PIXILS__HOOK_CONTEXT_H_
#define __PIXILS__HOOK_CONTEXT_H_

#include <pixils/frame_events.h>

namespace Pixils
{
  struct RenderContext;

  /**
   * Bundles per-frame input state and render context info into the single
   * context object passed as the second argument to all mode hooks.
   */
  struct HookContext
  {
    FrameEvents* events;
    const RenderContext* render;
  };

} // namespace Pixils

#endif /* __PIXILS__HOOK_CONTEXT_H_ */
