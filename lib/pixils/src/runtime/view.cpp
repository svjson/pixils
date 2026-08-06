
#include "pixils/runtime/view.h"

#include <pixils/benchmark/counters.h>
#include <pixils/runtime/state.h>
#include <pixils/ui/event.h>

#include <roo/runtime/dict.h>

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

    bool shared_policy(const View& view)
    {
      return view.component && view.state_policy == "shared" &&
             !view.component->ui_state_keys.empty();
    }

    bool map_contains_key(const Roo::sptr_val& value, const std::string& key)
    {
      return value && value->type == Roo::Value::Type::MAP &&
             Roo::Dict::contains_key(*value, key);
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

  void View::seed_ui_state_from_shared_state_policy()
  {
    if (!shared_policy(*this)) return;

    auto next_ui_state = ui_state && ui_state->type == Roo::Value::Type::MAP
                           ? Roo::Dict::shallow_copy(ui_state)
                           : Roo::map({});
    bool changed = false;
    for (const auto& key : component->ui_state_keys)
    {
      if (map_contains_key(state, key) &&
          (!map_contains_key(next_ui_state, key) || state_binding_controls_key(*this, key)))
      {
        Roo::Dict::set_property(next_ui_state,
                                Roo::keyword(key),
                                Roo::Dict::get_property(state, Roo::keyword(key)));
        changed = true;
      }
    }
    if (changed) set_ui_state_if_changed(next_ui_state);
  }

  void View::sync_state_from_shared_ui_state_policy()
  {
    if (!shared_policy(*this) || !ui_state || ui_state->type != Roo::Value::Type::MAP)
    {
      return;
    }

    auto next_state = state && state->type == Roo::Value::Type::MAP
                        ? Roo::Dict::shallow_copy(state)
                        : Roo::map({});
    bool changed = false;
    for (const auto& key : component->ui_state_keys)
    {
      if (map_contains_key(ui_state, key))
      {
        Roo::Dict::set_property(next_state,
                                Roo::keyword(key),
                                Roo::Dict::get_property(ui_state, Roo::keyword(key)));
        changed = true;
      }
    }
    if (changed) set_state_if_changed(next_state);
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
    pending_child_replacements.push_back(
      QueuedChildReplacement{child_id, std::move(child_slot)});
  }

  void View::queue_append_child(ChildSlot child_slot)
  {
    pending_child_appends.push_back(QueuedChildAppend{std::move(child_slot)});
  }
} // namespace Pixils::Runtime
