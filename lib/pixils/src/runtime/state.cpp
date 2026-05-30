
#include "pixils/runtime/state.h"

#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/runtime/view.h>

#include <roo/runtime/dict.h>

namespace Pixils::Runtime
{
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

    if (Pixils::Script::HostType::BIND_STATE.is_type_of(*binding))
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
      if (Pixils::Script::HostType::BIND_STATE.is_type_of(*val))
      {
        Roo::Dict::set_property(
          result,
          key,
          Roo::Dict::get_property_path(parent, Pixils::Runtime::bind_state_path(val)));
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

    if (Pixils::Script::HostType::BIND_STATE.is_type_of(*binding))
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
      if (Pixils::Script::HostType::BIND_STATE.is_type_of(*val))
      {
        auto child_val = Roo::Dict::get_property(child_state, key);
        result =
          Roo::Dict::assoc_in(result, Pixils::Runtime::bind_state_path(val), child_val);
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
      if (val && Script::HostType::BIND_STATE.is_type_of(*val))
      {
        has_binding = true;
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
