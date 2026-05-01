#ifndef PIXILS__UI__INTERACTION_DISPATCH_H
#define PIXILS__UI__INTERACTION_DISPATCH_H

#include <pixils/ui/mouse_state.h>

#include <memory>

namespace Lisple
{
  class Runtime;
}

namespace Pixils
{
  struct FrameEvents;
}

namespace Pixils::Runtime
{
  struct HookArguments;
  struct View;
} // namespace Pixils::Runtime

namespace Pixils::UI
{
  void dispatch_interactions(const std::shared_ptr<Pixils::Runtime::View>& root,
                             MouseState& mouse_state,
                             FrameEvents& events,
                             Pixils::Runtime::HookArguments& hook_args,
                             Lisple::Runtime& runtime);

} // namespace Pixils::UI

#endif /* PIXILS__UI__INTERACTION_DISPATCH_H */
