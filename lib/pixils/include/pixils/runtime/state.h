
#ifndef PIXILS__RUNTIME__STATE_H
#define PIXILS__RUNTIME__STATE_H

#include <roo/runtime/value.h>

namespace Pixils::Runtime
{
  struct View;

  /**
   * State binding path.
   */
  struct BindState
  {
    Roo::sptr_val_v path;
    bool writable = true;
    BindState() = default;
    explicit BindState(Roo::sptr_val_v p, bool w = true);
  };

  struct StateBinding
  {
    Roo::sptr_val binding = Roo::Constant::NIL;
    Roo::sptr_val initial_state = Roo::Constant::NIL;
  };

  /**
   * Create a state binding value for runtime-generated view definitions.
   * An empty path binds the child to the whole parent state.
   */
  Roo::sptr_val make_state_binding(Roo::sptr_val_v path = {}, bool writable = true);

  /**
   * Apply state_binding to produce the child state handed to hooks. For a
   * whole-path binding the child state is extracted from the parent at that
   * path. For a map binding, bound keys are overlaid on top of view.state
   * (which carries the literal/non-bound keys across frames). Unbound views
   * fully own their own view.state; initial_state is only the fallback before
   * the first local state has been established.
   */
  Roo::sptr_val extract_state(const Roo::sptr_val& parent,
                              const Pixils::Runtime::View& view);

  /**
   * Write bound keys from child_state back into parent state. Unbound views do
   * not merge into the parent at all. Bound views only touch the paths
   * declared in state_binding; non-bound keys remain in view.state.
   */
  Roo::sptr_val merge_state(const Roo::sptr_val& parent,
                            const Pixils::Runtime::View& view,
                            const Roo::sptr_val& child_state);

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

  const Roo::sptr_val_v& bind_state_path(const Roo::sptr_val& val);
  bool bind_state_writable(const Roo::sptr_val& val);

  /**
   * Returns true when a view's ordinary state binding controls a top-level key.
   * This is used by component shared-state migration policy so legacy
   * `:state {:key (ui/bind-state ...)}` component inputs can continue to feed
   * declared UI state keys while the component owns their continuity.
   */
  bool state_binding_controls_key(const Pixils::Runtime::View& view, const std::string& key);

  /**
   * Parse a raw :state value from a child slot entry into its binding and
   * literal-initial-state components. Three outcomes:
   *   - whole-path BindState -> binding = val, initial_state = NIL
   *   - map with BindState values -> binding = val, initial_state = literal keys only
   *   - plain map or NIL -> binding = NIL, initial_state = val
   */
  StateBinding parse_state_binding(const Roo::sptr_val& state_val);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__STATE_H */
