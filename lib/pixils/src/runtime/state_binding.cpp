#include "state_binding.h"

#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/runtime/state.h>

#include <roo/runtime/dict.h>

namespace Pixils::Runtime::StateBindings
{
  namespace
  {
    Roo::sptr_val child_property(const Roo::sptr_val& child_state, const Roo::sptr_val& key)
    {
      if (!child_state || child_state->type != Roo::Value::Type::MAP)
      {
        return Roo::Constant::NIL;
      }
      return Roo::Dict::get_property(child_state, key);
    }

  } // namespace

  bool is_binding(const Roo::sptr_val& value)
  {
    return value && Pixils::Script::HostType::BIND_STATE.is_type_of(*value);
  }

  bool contains_binding(const Roo::sptr_val& value)
  {
    if (is_binding(value)) return true;
    if (!value || value->type != Roo::Value::Type::MAP) return false;

    for (const auto& key : Roo::Dict::map_sptr_keys(value))
    {
      if (contains_binding(Roo::Dict::get_property(value, key))) return true;
    }
    return false;
  }

  bool contains_readable_binding(const Roo::sptr_val& value)
  {
    if (is_binding(value)) return Pixils::Runtime::bind_state_readable(value);
    if (!value || value->type != Roo::Value::Type::MAP) return false;

    for (const auto& key : Roo::Dict::map_sptr_keys(value))
    {
      if (contains_readable_binding(Roo::Dict::get_property(value, key))) return true;
    }
    return false;
  }

  Roo::sptr_val literal_value(const Roo::sptr_val& value)
  {
    if (!contains_binding(value)) return value;
    if (!value || value->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;

    auto literal = Roo::map({});
    for (const auto& key : Roo::Dict::map_sptr_keys(value))
    {
      auto child = Roo::Dict::get_property(value, key);
      if (!contains_binding(child))
      {
        Roo::Dict::set_property(literal, key, child);
      }
      else if (child && child->type == Roo::Value::Type::MAP)
      {
        Roo::Dict::set_property(literal, key, literal_value(child));
      }
    }
    return literal;
  }

  Roo::sptr_val resolve_value(const Roo::sptr_val& parent,
                              const Roo::sptr_val& current,
                              const Roo::sptr_val& binding)
  {
    if (is_binding(binding))
    {
      if (!Pixils::Runtime::bind_state_readable(binding)) return current;
      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return parent;
      return Roo::Dict::get_property_path(parent, path);
    }

    if (!binding || binding->type != Roo::Value::Type::MAP)
    {
      return current;
    }

    auto result = (current && current->type == Roo::Value::Type::MAP)
                    ? Roo::Dict::shallow_copy(current)
                    : Roo::map({});
    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto value = Roo::Dict::get_property(binding, key);
      auto current_has_key = current && current->type == Roo::Value::Type::MAP &&
                             Roo::Dict::contains_key(*current, key->str());
      if (contains_binding(value) && (contains_readable_binding(value) || current_has_key))
      {
        Roo::Dict::set_property(
          result,
          key,
          resolve_value(parent, Roo::Dict::get_property(result, key), value));
      }
    }
    return result;
  }

  Roo::sptr_val merge_pass(Roo::sptr_val result,
                           const Roo::sptr_val& binding,
                           const Roo::sptr_val& child_state,
                           bool whole_parent_bindings,
                           bool child_value_present)
  {
    if (is_binding(binding))
    {
      if (!Pixils::Runtime::bind_state_writable(binding)) return result;
      if (!Pixils::Runtime::bind_state_readable(binding) && !child_value_present)
      {
        return result;
      }

      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty())
      {
        return whole_parent_bindings && child_state ? child_state : result;
      }
      if (!whole_parent_bindings)
      {
        return Roo::Dict::assoc_in(result, path, child_state);
      }
      return result;
    }

    if (!binding || binding->type != Roo::Value::Type::MAP)
    {
      return result;
    }

    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto value = Roo::Dict::get_property(binding, key);
      if (contains_binding(value))
      {
        auto child_has_key = child_state && child_state->type == Roo::Value::Type::MAP &&
                             Roo::Dict::contains_key(*child_state, key->str());
        result = merge_pass(result,
                            value,
                            child_property(child_state, key),
                            whole_parent_bindings,
                            child_has_key);
      }
    }
    return result;
  }

} // namespace Pixils::Runtime::StateBindings
