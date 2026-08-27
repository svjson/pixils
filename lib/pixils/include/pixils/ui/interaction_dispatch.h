#ifndef PIXILS__UI__INTERACTION_DISPATCH_H
#define PIXILS__UI__INTERACTION_DISPATCH_H

#include <pixils/geom.h>
#include <pixils/ui/focus_state.h>
#include <pixils/ui/mouse_state.h>

#include <cstddef>
#include <memory>

namespace Roo
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
  void sync_focus_state(const std::shared_ptr<Pixils::Runtime::View>& root,
                        FocusState& focus_state);

  void dispatch_keyboard_events(const std::shared_ptr<Pixils::Runtime::View>& root,
                                FocusState& focus_state,
                                FrameEvents& events,
                                Pixils::Runtime::HookArguments& hook_args,
                                Roo::Runtime& runtime);

  bool dispatch_interactions(const std::shared_ptr<Pixils::Runtime::View>& root,
                             MouseState& mouse_state,
                             FocusState& focus_state,
                             FrameEvents& events,
                             Pixils::Runtime::HookArguments& hook_args,
                             Roo::Runtime& runtime);

  /** Return the deepest-to-root hit-chain length at a global point. */
  size_t interaction_hit_depth(const std::shared_ptr<Pixils::Runtime::View>& root,
                               const Point& point);

} // namespace Pixils::UI

#endif /* PIXILS__UI__INTERACTION_DISPATCH_H */
