#include "pixils/runtime/component_state.h"
#include "state_binding.h"
#include <pixils/benchmark/counters.h>
#include <pixils/runtime/component.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>

#include <roo/runtime/dict.h>

namespace Pixils::Runtime
{
  namespace
  {
    bool state_values_equal(const Roo::sptr_val& lhs, const Roo::sptr_val& rhs)
    {
      if (lhs == rhs) return true;
      if (!lhs || !rhs || lhs->type != rhs->type) return false;
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

    bool state_binding_controls_key(const Pixils::Runtime::View& view,
                                    const std::string& key)
    {
      const auto& binding = view.state_binding;
      if (!binding || binding->type == Roo::Value::Type::NIL) return false;
      if (StateBindings::is_binding(binding))
      {
        return Pixils::Runtime::bind_state_readable(binding);
      }
      if (binding->type != Roo::Value::Type::MAP) return false;

      auto value = Roo::Dict::get_property(binding, Roo::keyword(key));
      return StateBindings::contains_readable_binding(value);
    }

  } // namespace

  bool View::set_state_from_child_bindings(const Roo::sptr_val& next_state,
                                           Roo::Runtime& runtime)
  {
    auto previous_state = state;
    auto state_changed = set_state_if_changed(next_state);
    auto ui_generation = state_generation;
    sync_ui_state_from_child_bindings(previous_state, next_state, runtime);
    return state_changed || state_generation != ui_generation;
  }

  void View::sync_ui_state_from_child_bindings(const Roo::sptr_val& previous_state,
                                               const Roo::sptr_val& next_state,
                                               Roo::Runtime& runtime)
  {
    if (!shared_policy(*this) || !next_state || next_state->type != Roo::Value::Type::MAP)
    {
      return;
    }

    auto next_ui_state = ui_state && ui_state->type == Roo::Value::Type::MAP
                           ? Roo::Dict::shallow_copy(ui_state)
                           : Roo::map({});
    bool changed = false;
    for (const auto& key : component->ui_state_keys)
    {
      if (!map_contains_key(next_state, key)) continue;

      auto key_value = Roo::keyword(key);
      auto next_value = Roo::Dict::get_property(next_state, key_value);
      auto had_previous_value = map_contains_key(previous_state, key);
      auto previous_value = had_previous_value
                              ? Roo::Dict::get_property(previous_state, key_value)
                              : Roo::Constant::NIL;
      PIXILS_BENCHMARK_COUNT(view_state_equality_checks);
      if (!had_previous_value || !state_values_equal(previous_value, next_value))
      {
        Roo::Dict::set_property(next_ui_state, key_value, next_value);
        changed = true;
      }
    }
    if (changed) transition_component_ui_state(*this, next_ui_state, runtime);
  }

  void View::seed_ui_state_from_shared_state_policy(Roo::Runtime& runtime)
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
    if (changed) transition_component_ui_state(*this, next_ui_state, runtime);
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

} // namespace Pixils::Runtime
