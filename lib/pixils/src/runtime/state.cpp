
#include "pixils/runtime/state.h"

#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/runtime/view.h>

#include <roo/runtime/dict.h>

namespace Pixils::Runtime
{
  namespace
  {
    bool is_bind_state(const Roo::sptr_val& val)
    {
      return val && Pixils::Script::HostType::BIND_STATE.is_type_of(*val);
    }

    bool contains_state_binding(const Roo::sptr_val& val)
    {
      if (is_bind_state(val)) return true;
      if (!val || val->type != Roo::Value::Type::MAP) return false;

      for (const auto& key : Roo::Dict::map_sptr_keys(val))
      {
        if (contains_state_binding(Roo::Dict::get_property(val, key))) return true;
      }
      return false;
    }

    Roo::sptr_val literal_state_value(const Roo::sptr_val& val)
    {
      if (!contains_state_binding(val)) return val;
      if (!val || val->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;

      auto literal = Roo::map({});
      for (const auto& key : Roo::Dict::map_sptr_keys(val))
      {
        auto child = Roo::Dict::get_property(val, key);
        if (!contains_state_binding(child))
        {
          Roo::Dict::set_property(literal, key, child);
        }
        else if (child && child->type == Roo::Value::Type::MAP)
        {
          Roo::Dict::set_property(literal, key, literal_state_value(child));
        }
      }
      return literal;
    }

    Roo::sptr_val resolve_state_binding_value(const Roo::sptr_val& parent,
                                              const Roo::sptr_val& current,
                                              const Roo::sptr_val& binding)
    {
      if (is_bind_state(binding))
      {
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
        auto val = Roo::Dict::get_property(binding, key);
        if (contains_state_binding(val))
        {
          Roo::Dict::set_property(
            result,
            key,
            resolve_state_binding_value(parent, Roo::Dict::get_property(result, key), val));
        }
      }
      return result;
    }

  } // namespace

  BindState::BindState(Roo::sptr_val_v p)
    : path(std::move(p))
  {
  }

  Roo::sptr_val extract_state(const Roo::sptr_val& parent,
                                 const Pixils::Runtime::View& view)
  {
    const auto& binding = view.state_binding;

    if (!binding || binding->type == Roo::Value::Type::NIL)
    {
      if (view.state && view.state->type != Roo::Value::Type::NIL) return view.state;
      return view.initial_state;
    }

    if (is_bind_state(binding))
    {
      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return parent;
      return Roo::Dict::get_property_path(parent,
                                             path);
    }

    auto result = (view.state && view.state->type != Roo::Value::Type::NIL)
                    ? Roo::Dict::shallow_copy(view.state)
                    : Roo::map({});
    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto val = Roo::Dict::get_property(binding, key);
      if (contains_state_binding(val))
      {
        Roo::Dict::set_property(
          result,
          key,
          resolve_state_binding_value(parent, Roo::Dict::get_property(result, key), val));
      }
    }
    return result;
  }

  Roo::sptr_val merge_state(const Roo::sptr_val& parent,
                               const Pixils::Runtime::View& view,
                               const Roo::sptr_val& child_state)
  {
    const auto& binding = view.state_binding;

    if (!binding || binding->type == Roo::Value::Type::NIL)
    {
      return parent;
    }

    if (is_bind_state(binding))
    {
      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return child_state;
      return Roo::Dict::assoc_in(parent,
                                    path,
                                    child_state);
    }

    auto result = parent;
    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto val = Roo::Dict::get_property(binding, key);
      if (is_bind_state(val) && Pixils::Runtime::bind_state_path(val).empty())
      {
        auto child_val = Roo::Dict::get_property(child_state, key);
        if (child_val) result = child_val;
      }
    }

    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto val = Roo::Dict::get_property(binding, key);
      if (is_bind_state(val))
      {
        const auto& path = Pixils::Runtime::bind_state_path(val);
        if (path.empty()) continue;

        auto child_val = Roo::Dict::get_property(child_state, key);
        result = Roo::Dict::assoc_in(result, path, child_val);
      }
    }
    return result;
  }

  const Roo::sptr_val_v& bind_state_path(const Roo::sptr_val& val)
  {
    return Roo::obj<Runtime::BindState>(*val).path;
  }

  StateBinding parse_state_binding(const Roo::sptr_val& state_val)
  {
    if (state_val && Script::HostType::BIND_STATE.is_type_of(*state_val))
    {
      return {state_val, Roo::Constant::NIL};
    }

    if (!state_val || state_val->type == Roo::Value::Type::NIL) return {};

    Roo::sptr_val literal = Roo::map({});
    bool has_binding = false;
    for (const auto& key : Roo::Dict::map_sptr_keys(state_val))
    {
      auto val = Roo::Dict::get_property(state_val, key);
      if (contains_state_binding(val))
      {
        has_binding = true;
        if (val && val->type == Roo::Value::Type::MAP)
        {
          Roo::Dict::set_property(literal, key, literal_state_value(val));
        }
      }
      else
      {
        Roo::Dict::set_property(literal, key, val);
      }
    }

    if (has_binding) return {state_val, literal};

    return {Roo::Constant::NIL, state_val};
  }

} // namespace Pixils::Runtime
