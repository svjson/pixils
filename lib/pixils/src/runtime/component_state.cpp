#include "pixils/runtime/component_state.h"

#include "state_binding.h"
#include <pixils/runtime/component.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>

#include <algorithm>
#include <roo/exception.h>
#include <roo/runtime/dict.h>

namespace Pixils::Runtime
{
  namespace
  {
    bool declared_component_ui_key(const Pixils::Runtime::View& view,
                                   const Roo::sptr_val& key)
    {
      if (!view.component || !key) return false;
      const auto& keys = view.component->ui_state_keys;
      return std::find(keys.begin(), keys.end(), key->str()) != keys.end();
    }

    Roo::sptr_val declared_component_ui_state(const Pixils::Runtime::View& view,
                                              const Roo::sptr_val& state)
    {
      auto result = Roo::map({});
      if (!state || state->type != Roo::Value::Type::MAP || !view.component)
      {
        return result;
      }

      for (const auto& key : Roo::Dict::map_sptr_keys(state))
      {
        if (declared_component_ui_key(view, key))
        {
          Roo::Dict::set_property(result, key, Roo::Dict::get_property(state, key));
        }
      }
      return result;
    }

    Roo::sptr_val ui_property(const Roo::sptr_val& ui_state, const Roo::sptr_val& key)
    {
      if (!ui_state || ui_state->type != Roo::Value::Type::MAP)
      {
        return Roo::Constant::NIL;
      }
      return Roo::Dict::get_property(ui_state, key);
    }

    Roo::sptr_val extract_bound_component_ui_state(const Roo::sptr_val& parent,
                                                   const Pixils::Runtime::View& view,
                                                   const Roo::sptr_val& binding,
                                                   const Roo::sptr_val& current)
    {
      if (!binding || binding->type == Roo::Value::Type::NIL) return current;

      auto result = current && current->type == Roo::Value::Type::MAP
                      ? Roo::Dict::shallow_copy(current)
                      : Roo::map({});

      if (StateBindings::is_binding(binding))
      {
        if (!Pixils::Runtime::bind_state_readable(binding)) return current;
        const auto& path = Pixils::Runtime::bind_state_path(binding);
        auto bound_value =
          path.empty() ? parent : Roo::Dict::get_property_path(parent, path);
        auto declared = declared_component_ui_state(view, bound_value);
        for (const auto& key : Roo::Dict::map_sptr_keys(declared))
        {
          Roo::Dict::set_property(result, key, Roo::Dict::get_property(declared, key));
        }
        return result;
      }

      if (binding->type != Roo::Value::Type::MAP) return result;

      for (const auto& key : Roo::Dict::map_sptr_keys(binding))
      {
        auto value = Roo::Dict::get_property(binding, key);
        auto result_has_key = result && result->type == Roo::Value::Type::MAP &&
                              Roo::Dict::contains_key(*result, key->str());
        if (declared_component_ui_key(view, key) && StateBindings::contains_binding(value) &&
            (StateBindings::contains_readable_binding(value) || result_has_key))
        {
          Roo::Dict::set_property(
            result,
            key,
            StateBindings::resolve_value(parent,
                                         Roo::Dict::get_property(result, key),
                                         value));
        }
      }
      return result;
    }

    Roo::sptr_val merge_bound_component_ui_state(const Roo::sptr_val& parent,
                                                 const Pixils::Runtime::View& view,
                                                 const Roo::sptr_val& binding,
                                                 const Roo::sptr_val& ui_state)
    {
      if (!binding || binding->type == Roo::Value::Type::NIL) return parent;

      if (StateBindings::is_binding(binding))
      {
        if (!Pixils::Runtime::bind_state_writable(binding)) return parent;

        const auto& path = Pixils::Runtime::bind_state_path(binding);
        auto value = declared_component_ui_state(view, ui_state);
        if (path.empty()) return value;
        return Roo::Dict::assoc_in(parent, path, value);
      }

      if (binding->type != Roo::Value::Type::MAP) return parent;

      auto result = parent;
      for (const auto& key : Roo::Dict::map_sptr_keys(binding))
      {
        auto value = Roo::Dict::get_property(binding, key);
        if (declared_component_ui_key(view, key) && StateBindings::contains_binding(value))
        {
          auto child_value_present = ui_state && ui_state->type == Roo::Value::Type::MAP &&
                                     Roo::Dict::contains_key(*ui_state, key->str());
          result = StateBindings::merge_pass(result,
                                             value,
                                             ui_property(ui_state, key),
                                             true,
                                             child_value_present);
        }
      }
      for (const auto& key : Roo::Dict::map_sptr_keys(binding))
      {
        auto value = Roo::Dict::get_property(binding, key);
        if (declared_component_ui_key(view, key) && StateBindings::contains_binding(value))
        {
          auto child_value_present = ui_state && ui_state->type == Roo::Value::Type::MAP &&
                                     Roo::Dict::contains_key(*ui_state, key->str());
          result = StateBindings::merge_pass(result,
                                             value,
                                             ui_property(ui_state, key),
                                             false,
                                             child_value_present);
        }
      }
      return result;
    }

  } // namespace

  Roo::sptr_val extract_component_ui_state(const Roo::sptr_val& parent,
                                           const Pixils::Runtime::View& view)
  {
    if (!view.component)
    {
      return view.ui_state;
    }

    auto result = extract_bound_component_ui_state(parent,
                                                   view,
                                                   view.component_state_binding,
                                                   view.ui_state);
    if (view.ui_state_binding && view.ui_state_binding->type != Roo::Value::Type::NIL)
    {
      if (!view.parent || !view.parent->has_component_ui_state())
      {
        throw Roo::TypeError("Child :ui-state bindings require a component parent");
      }
      result = extract_bound_component_ui_state(view.parent->ui_state,
                                                view,
                                                view.ui_state_binding,
                                                result);
    }
    return result;
  }

  Roo::sptr_val merge_component_ui_state(const Roo::sptr_val& parent,
                                         const Pixils::Runtime::View& view,
                                         const Roo::sptr_val& ui_state)
  {
    if (!view.component) return parent;
    return merge_bound_component_ui_state(parent,
                                          view,
                                          view.component_state_binding,
                                          ui_state);
  }

  bool View::set_ui_state_from_child_bindings(const View& child, Roo::Runtime& runtime)
  {
    if (!child.ui_state_binding || child.ui_state_binding->type == Roo::Value::Type::NIL)
    {
      return false;
    }
    if (!has_component_ui_state())
    {
      throw Roo::TypeError("Child :ui-state bindings require a component parent");
    }

    return transition_component_ui_state(
      *this,
      merge_bound_component_ui_state(ui_state,
                                     child,
                                     child.ui_state_binding,
                                     child.ui_state),
      runtime);
  }

} // namespace Pixils::Runtime
