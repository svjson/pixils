
#ifndef PIXILS__RUNTIME__STATE_H
#define PIXILS__RUNTIME__STATE_H

#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  struct View;

  /**
   * State binding path.
   */
  struct BindState
  {
    Lisple::sptr_rtval_v path;
    BindState() = default;
    explicit BindState(Lisple::sptr_rtval_v p);
  };

  struct StateBinding
  {
    Lisple::sptr_rtval binding = Lisple::Constant::NIL;
    Lisple::sptr_rtval initial_state = Lisple::Constant::NIL;
  };

  /**
   * Apply state_binding to produce the child state handed to hooks. For a
   * whole-path binding the child state is extracted from the parent at that
   * path. For a map binding, bound keys are overlaid on top of view.state
   * (which carries the literal/non-bound keys across frames). For unbound
   * views the id-keyword path is used and initial_state is the fallback.
   */
  Lisple::sptr_rtval extract_state(const Lisple::sptr_rtval& parent,
                                   const Pixils::Runtime::View& view);

  /**
   * Write bound keys from child_state back into parent state. Unbound views
   * store the full child state under the id keyword. Bound views only touch
   * the paths declared in state_binding; non-bound keys remain in view.state.
   */
  Lisple::sptr_rtval merge_state(const Lisple::sptr_rtval& parent,
                                 const Pixils::Runtime::View& view,
                                 const Lisple::sptr_rtval& child_state);

  const Lisple::sptr_rtval_v& bind_state_path(const Lisple::sptr_rtval& val);

  /**
   * Parse a raw :state value from a child slot entry into its binding and
   * literal-initial-state components. Three outcomes:
   *   - whole-path BindState -> binding = val, initial_state = NIL
   *   - map with BindState values -> binding = val, initial_state = literal keys only
   *   - plain map or NIL -> binding = NIL, initial_state = val
   */
  StateBinding parse_state_binding(const Lisple::sptr_rtval& state_val);

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__STATE_H */
