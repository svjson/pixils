#include <pixils/runtime/view_state_binding.h>

#include <pixils/runtime/component_state.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>

#include <roo/runtime.h>

namespace Pixils::Runtime
{
  namespace
  {
    bool has_binding(const Roo::sptr_val& binding)
    {
      return binding && binding->type != Roo::Value::Type::NIL;
    }
  } // namespace

  bool has_bound_view_state(const View& view)
  {
    return has_binding(view.state_binding) || has_binding(view.component_state_binding) ||
           has_binding(view.ui_state_binding);
  }

  bool pull_bound_view_state(View& view, Roo::Runtime& runtime)
  {
    auto generation = view.state_generation;

    if (view.parent && has_binding(view.state_binding))
    {
      view.set_state_if_changed(extract_state(view.parent->state, view));
    }

    if (view.has_component_ui_state())
    {
      if (view.parent)
      {
        transition_component_ui_state(
          view,
          extract_component_ui_state(view.parent->state, view),
          runtime);
      }
      view.seed_ui_state_from_shared_state_policy(runtime);
      view.sync_state_from_shared_ui_state_policy();
    }

    return view.state_generation != generation;
  }

  bool push_bound_view_state(View& view, Roo::Runtime& runtime)
  {
    if (!view.parent || !has_bound_view_state(view)) return false;

    auto& parent = *view.parent;
    auto merged = merge_component_ui_state(merge_state(parent.state, view, view.state),
                                           view,
                                           view.ui_state);
    bool changed = parent.set_state_from_child_bindings(merged, runtime);
    return parent.set_ui_state_from_child_bindings(view, runtime) || changed;
  }

} // namespace Pixils::Runtime
