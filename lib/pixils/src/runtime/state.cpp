
#include "pixils/runtime/state.h"

#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/runtime/view.h>

#include <lisple/runtime/dict.h>

namespace Pixils::Runtime
{
  BindState::BindState(Lisple::sptr_val_v p)
    : path(std::move(p))
  {
  }

  Lisple::sptr_val extract_state(const Lisple::sptr_val& parent,
                                 const Pixils::Runtime::View& view)
  {
    const auto& binding = view.state_binding;

    if (!binding || binding->type == Lisple::Value::Type::NIL)
    {
      if (view.state && view.state->type != Lisple::Value::Type::NIL) return view.state;
      return view.initial_state;
    }

    if (Pixils::Script::HostType::BIND_STATE.is_type_of(*binding))
    {
      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return parent;
      return Lisple::Dict::get_property_path(parent,
                                             path);
    }

    auto result = (view.state && view.state->type != Lisple::Value::Type::NIL)
                    ? Lisple::Dict::shallow_copy(view.state)
                    : Lisple::map({});
    for (const auto& key : Lisple::Dict::map_sptr_keys(binding))
    {
      auto val = Lisple::Dict::get_property(binding, key);
      if (Pixils::Script::HostType::BIND_STATE.is_type_of(*val))
      {
        Lisple::Dict::set_property(
          result,
          key,
          Lisple::Dict::get_property_path(parent, Pixils::Runtime::bind_state_path(val)));
      }
    }
    return result;
  }

  Lisple::sptr_val merge_state(const Lisple::sptr_val& parent,
                               const Pixils::Runtime::View& view,
                               const Lisple::sptr_val& child_state)
  {
    const auto& binding = view.state_binding;

    if (!binding || binding->type == Lisple::Value::Type::NIL)
    {
      return parent;
    }

    if (Pixils::Script::HostType::BIND_STATE.is_type_of(*binding))
    {
      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return child_state;
      return Lisple::Dict::assoc_in(parent,
                                    path,
                                    child_state);
    }

    auto result = parent;
    for (const auto& key : Lisple::Dict::map_sptr_keys(binding))
    {
      auto val = Lisple::Dict::get_property(binding, key);
      if (Pixils::Script::HostType::BIND_STATE.is_type_of(*val))
      {
        auto child_val = Lisple::Dict::get_property(child_state, key);
        result =
          Lisple::Dict::assoc_in(result, Pixils::Runtime::bind_state_path(val), child_val);
      }
    }
    return result;
  }

  const Lisple::sptr_val_v& bind_state_path(const Lisple::sptr_val& val)
  {
    return Lisple::obj<Runtime::BindState>(*val).path;
  }

  StateBinding parse_state_binding(const Lisple::sptr_val& state_val)
  {
    if (state_val && Script::HostType::BIND_STATE.is_type_of(*state_val))
    {
      return {state_val, Lisple::Constant::NIL};
    }

    if (!state_val || state_val->type == Lisple::Value::Type::NIL) return {};

    Lisple::sptr_val literal = Lisple::map({});
    bool has_binding = false;
    for (const auto& key : Lisple::Dict::map_sptr_keys(state_val))
    {
      auto val = Lisple::Dict::get_property(state_val, key);
      if (val && Script::HostType::BIND_STATE.is_type_of(*val))
      {
        has_binding = true;
      }
      else
      {
        Lisple::Dict::set_property(literal, key, val);
      }
    }

    if (has_binding) return {state_val, literal};

    return {Lisple::Constant::NIL, state_val};
  }

} // namespace Pixils::Runtime
