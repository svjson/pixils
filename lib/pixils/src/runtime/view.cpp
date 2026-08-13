
#include "pixils/runtime/view.h"

#include <pixils/benchmark/counters.h>
#include <pixils/ui/event.h>

namespace Pixils::Runtime
{
  namespace
  {
    bool rtval_equal(const Roo::sptr_val& lhs, const Roo::sptr_val& rhs)
    {
      PIXILS_BENCHMARK_COUNT(view_state_equality_checks);
      if (lhs == rhs) return true;
      if (!lhs || !rhs) return false;
      if (lhs->type != rhs->type) return false;

      return *lhs == *rhs;
    }

  } // namespace

  void View::set_parent(View* next_parent)
  {
    if (parent == next_parent) return;
    parent = next_parent;
    touch_subtree_generation();
  }

  void View::touch_subtree_generation()
  {
    subtree_generation++;
    if (parent) parent->touch_subtree_generation();
  }

  void View::mark_state_changed()
  {
    state_generation++;
    touch_subtree_generation();
  }

  void View::mark_interaction_changed()
  {
    interaction_generation++;
    touch_subtree_generation();
  }

  void View::mark_children_changed()
  {
    children_generation++;
    touch_subtree_generation();
  }

  void View::mark_style_changed()
  {
    style_generation++;
    touch_subtree_generation();
  }

  bool View::set_state_if_changed(const Roo::sptr_val& next_state)
  {
    if (rtval_equal(state, next_state))
    {
      PIXILS_BENCHMARK_COUNT(view_state_assignments_preserved);
      return false;
    }

    PIXILS_BENCHMARK_COUNT(view_state_assignments_replaced);
    state = next_state;
    mark_state_changed();
    return true;
  }

  bool View::set_ui_state_if_changed(const Roo::sptr_val& next_state)
  {
    if (rtval_equal(ui_state, next_state))
    {
      return false;
    }

    ui_state = next_state;
    mark_state_changed();
    return true;
  }

  bool View::has_component_ui_state() const
  {
    return component != nullptr;
  }

  void View::emit_event(const CustomEvent& event)
  {
    this->emitted_events.push_back(event);
  }

  void View::drain_events(std::vector<CustomEvent>& collected)
  {
    for (auto& event : this->emitted_events)
    {
      collected.push_back(event);
    }
    this->emitted_events.clear();
  }

  void View::queue_replace_child(const std::string& child_id, ChildSlot child_slot)
  {
    QueuedChildMutation mutation;
    mutation.type = ChildMutationType::REPLACE;
    mutation.child_id = child_id;
    mutation.child_slot = std::move(child_slot);
    pending_child_mutations.push_back(std::move(mutation));
  }

  void View::queue_append_child(ChildSlot child_slot)
  {
    QueuedChildMutation mutation;
    mutation.type = ChildMutationType::APPEND;
    mutation.child_slot = std::move(child_slot);
    pending_child_mutations.push_back(std::move(mutation));
  }

  void View::queue_remove_child(const std::string& child_id)
  {
    QueuedChildMutation mutation;
    mutation.type = ChildMutationType::REMOVE;
    mutation.child_id = child_id;
    pending_child_mutations.push_back(std::move(mutation));
  }

  void View::queue_reconcile_children(std::vector<ChildSlot> child_slots)
  {
    QueuedChildMutation mutation;
    mutation.type = ChildMutationType::RECONCILE;
    mutation.child_slots = std::move(child_slots);
    pending_child_mutations.push_back(std::move(mutation));
  }
} // namespace Pixils::Runtime
