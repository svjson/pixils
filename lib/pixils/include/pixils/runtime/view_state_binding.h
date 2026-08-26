#ifndef PIXILS__RUNTIME__VIEW_STATE_BINDING_H
#define PIXILS__RUNTIME__VIEW_STATE_BINDING_H

namespace Roo
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct View;

  /**
   * Synchronize a view's readable ordinary-state, component-state, and
   * UI-state bindings from its parent.
   */
  bool pull_bound_view_state(View& view, Roo::Runtime& runtime);

  /**
   * Synchronize a view's writable ordinary-state, component-state, and
   * UI-state bindings into its parent.
   */
  bool push_bound_view_state(View& view, Roo::Runtime& runtime);

  /**
   * Whether a view has any ordinary-state, component-state, or UI-state
   * binding to its parent.
   */
  bool has_bound_view_state(const View& view);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__VIEW_STATE_BINDING_H */
