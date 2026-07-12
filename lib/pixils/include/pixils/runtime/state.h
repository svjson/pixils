
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

  const Roo::sptr_val_v& bind_state_path(const Roo::sptr_val& val);
  bool bind_state_writable(const Roo::sptr_val& val);

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
