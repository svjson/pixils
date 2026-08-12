#ifndef PIXILS__RUNTIME__COMPONENT_STATE_H
#define PIXILS__RUNTIME__COMPONENT_STATE_H

#include <roo/runtime/value.h>

namespace Roo
{
  class Context;
  class Runtime;
} // namespace Roo

namespace Pixils::Runtime
{
  struct View;

  /**
   * Apply a component view's `:state {:component ...}` binding channel to its
   * UI state. Component-state bindings are resolved from the parent state just
   * like ordinary child state bindings, but they target declared component UI
   * state keys instead of the view's application state.
   */
  Roo::sptr_val extract_component_ui_state(const Roo::sptr_val& parent,
                                           const Pixils::Runtime::View& view);

  /**
   * Write writable `:state {:component ...}` bindings from a component view's
   * UI state back into the parent application state.
   */
  Roo::sptr_val merge_component_ui_state(const Roo::sptr_val& parent,
                                         const Pixils::Runtime::View& view,
                                         const Roo::sptr_val& ui_state);

  /**
   * Resolve component UI-state models against `candidate` and commit the
   * resulting state. Models run only when their owned or dependency keys
   * differ from the current UI state. During initialization every model runs
   * against an empty current state.
   */
  bool transition_component_ui_state(Pixils::Runtime::View& view,
                                     const Roo::sptr_val& candidate,
                                     Roo::Context& ctx,
                                     bool initializing = false);

  bool transition_component_ui_state(Pixils::Runtime::View& view,
                                     const Roo::sptr_val& candidate,
                                     Roo::Runtime& runtime,
                                     bool initializing = false);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__COMPONENT_STATE_H */
