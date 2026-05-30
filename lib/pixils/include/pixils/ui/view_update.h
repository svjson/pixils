#ifndef PIXILS__UI__VIEW_UPDATE_H
#define PIXILS__UI__VIEW_UPDATE_H

#include <pixils/geom.h>
#include <pixils/ui/focus_state.h>
#include <pixils/ui/mouse_state.h>

#include <memory>

namespace Roo
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct HookArguments;
  struct View;
} // namespace Pixils::Runtime

namespace Pixils::UI
{
  void update_view_tree(const std::shared_ptr<Runtime::View>& root,
                        const MouseState& mouse_state,
                        const FocusState& focus_state,
                        const Point& mouse_pos,
                        Runtime::HookArguments& hook_args,
                        Roo::Runtime& runtime);

  void refresh_view_interaction_tree(const std::shared_ptr<Runtime::View>& root,
                                     const MouseState& mouse_state,
                                     const FocusState& focus_state,
                                     const Point& mouse_pos);
}

#endif /* PIXILS__UI__VIEW_UPDATE_H */
