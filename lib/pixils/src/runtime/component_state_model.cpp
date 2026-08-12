#include "pixils/runtime/component_state.h"
#include <pixils/runtime/component.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/event.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exception.h>
#include <roo/exec.h>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>

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

  } // namespace

  bool transition_component_ui_state(Pixils::Runtime::View& view,
                                     const Roo::sptr_val& candidate,
                                     Roo::Context& ctx,
                                     bool initializing)
  {
    if (!candidate || candidate->type != Roo::Value::Type::MAP)
    {
      throw Roo::TypeError("Component UI state transition requires a map");
    }
    if (!view.component)
    {
      throw Roo::TypeError("UI state models require a component view");
    }

    auto current =
      initializing || !view.ui_state || view.ui_state->type != Roo::Value::Type::MAP
        ? Roo::map({})
        : view.ui_state;
    auto next = Roo::Dict::shallow_copy(candidate);

    Roo::sptr_val_v changed_key_values;
    std::vector<std::string> changed_keys;
    std::vector<CustomEvent> change_events;
    auto collect_changed_keys = [&](const Roo::sptr_val& state)
    {
      for (const auto& key : Roo::Dict::map_sptr_keys(state))
      {
        const auto& name = key->str();
        if (std::find(changed_keys.begin(), changed_keys.end(), name) != changed_keys.end())
        {
          continue;
        }

        auto current_has = Roo::Dict::contains_key(*current, name);
        auto next_has = Roo::Dict::contains_key(*next, name);
        if (initializing || current_has != next_has ||
            !state_values_equal(Roo::Dict::get_property(current, key),
                                Roo::Dict::get_property(next, key)))
        {
          changed_keys.push_back(name);
          changed_key_values.push_back(Roo::keyword(name));
        }
      }
    };
    collect_changed_keys(current);
    collect_changed_keys(next);

    for (const auto& model : view.component->ui_state_models)
    {
      auto relevant =
        initializing || std::any_of(changed_keys.begin(),
                                    changed_keys.end(),
                                    [&](const auto& key)
                                    {
                                      return std::find(model.owned_keys.begin(),
                                                       model.owned_keys.end(),
                                                       key) != model.owned_keys.end() ||
                                             std::find(model.dependency_keys.begin(),
                                                       model.dependency_keys.end(),
                                                       key) != model.dependency_keys.end();
                                    });
      if (!relevant) continue;

      Roo::sptr_val_v args = {current, next, Roo::vector(changed_key_values)};
      auto corrections = model.transition->exec().execute(ctx, args);
      if (!corrections || corrections->type != Roo::Value::Type::MAP)
      {
        throw Roo::TypeError("UI state model :transition must return a map");
      }

      for (const auto& key : Roo::Dict::map_sptr_keys(corrections))
      {
        if (std::find(model.owned_keys.begin(), model.owned_keys.end(), key->str()) ==
            model.owned_keys.end())
        {
          throw Roo::TypeError("UI state model returned unowned key :" + key->str());
        }
        Roo::Dict::set_property(next, key, Roo::Dict::get_property(corrections, key));
      }

      if (!initializing && model.change_event->type != Roo::Value::Type::NIL)
      {
        auto payload = Roo::map({});
        bool owned_state_changed = false;
        for (const auto& key : model.owned_keys)
        {
          auto key_value = Roo::keyword(key);
          auto current_has = Roo::Dict::contains_key(*current, key);
          auto next_has = Roo::Dict::contains_key(*next, key);
          auto next_value = Roo::Dict::get_property(next, key_value);
          if (current_has != next_has ||
              !state_values_equal(Roo::Dict::get_property(current, key_value), next_value))
          {
            owned_state_changed = true;
          }
          if (next_has)
          {
            Roo::Dict::set_property(payload, key_value, next_value);
          }
        }
        if (owned_state_changed)
        {
          auto source_mode =
            view.definition ? Roo::symbol(view.definition->name) : Roo::Constant::NIL;
          change_events.emplace_back(model.change_event, payload, source_mode);
        }
      }
    }

    auto changed = view.set_ui_state_if_changed(next);
    if (changed)
    {
      for (const auto& event : change_events)
      {
        view.emit_event(event);
      }
    }
    return changed;
  }

  bool transition_component_ui_state(Pixils::Runtime::View& view,
                                     const Roo::sptr_val& candidate,
                                     Roo::Runtime& runtime,
                                     bool initializing)
  {
    Roo::Context ctx(runtime);
    return transition_component_ui_state(view, candidate, ctx, initializing);
  }

} // namespace Pixils::Runtime
