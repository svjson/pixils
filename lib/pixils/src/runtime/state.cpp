
#include "pixils/runtime/state.h"

#include <pixils/binding/ui_namespace.h>
#include <pixils/runtime/view.h>

#include <lisple/runtime/dict.h>

namespace Pixils::Runtime
{
  BindState::BindState(Lisple::sptr_rtval_v p)
    : path(std::move(p))
  {
  }

  Lisple::sptr_rtval extract_state(const Lisple::sptr_rtval& parent,
                                   const Pixils::Runtime::View& view)
  {
    const auto& binding = view.state_binding;

    if (!binding || binding->type == Lisple::RTValue::Type::NIL)
    {
      if (view.state && view.state->type != Lisple::RTValue::Type::NIL) return view.state;
      return view.initial_state;
    }

    if (Pixils::Script::HostType::BIND_STATE.is_type_of(*binding))
      return Lisple::Dict::get_property_path(parent,
                                             Pixils::Runtime::bind_state_path(binding));

    auto result = (view.state && view.state->type != Lisple::RTValue::Type::NIL)
                    ? Lisple::Dict::shallow_copy(view.state)
                    : Lisple::RTValue::map({});
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

  Lisple::sptr_rtval merge_state(const Lisple::sptr_rtval& parent,
                                 const Pixils::Runtime::View& view,
                                 const Lisple::sptr_rtval& child_state)
  {
    const auto& binding = view.state_binding;

    if (!binding || binding->type == Lisple::RTValue::Type::NIL)
    {
      return parent;
    }

    if (Pixils::Script::HostType::BIND_STATE.is_type_of(*binding))
      return Lisple::Dict::assoc_in(parent,
                                    Pixils::Runtime::bind_state_path(binding),
                                    child_state);

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

  const Lisple::sptr_rtval_v& bind_state_path(const Lisple::sptr_rtval& val)
  {
    return Lisple::obj<Runtime::BindState>(*val).path;
  }

  StateBinding parse_state_binding(const Lisple::sptr_rtval& state_val)
  {
    if (state_val && Script::HostType::BIND_STATE.is_type_of(*state_val))
    {
      return {state_val, Lisple::Constant::NIL};
    }

    if (!state_val || state_val->type == Lisple::RTValue::Type::NIL) return {};

    Lisple::sptr_rtval literal = Lisple::RTValue::map({});
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
