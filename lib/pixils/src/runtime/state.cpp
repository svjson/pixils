#include "pixils/runtime/state.h"

#include "state_binding.h"
#include <pixils/binding/ui/ui_namespace.h>
#include <pixils/runtime/view.h>

#include <roo/host/object.h>
#include <roo/runtime/dict.h>

namespace Pixils::Runtime
{
  BindState::BindState(Roo::sptr_val_v p, bool w)
    : path(std::move(p))
    , writable(w)
  {
  }

  Roo::sptr_val make_state_binding(Roo::sptr_val_v path, bool writable)
  {
    return Script::BindStateAdapter::make_unique(BindState(std::move(path), writable));
  }

  Roo::sptr_val extract_state(const Roo::sptr_val& parent, const Pixils::Runtime::View& view)
  {
    const auto& binding = view.state_binding;

    if (!binding || binding->type == Roo::Value::Type::NIL)
    {
      if (view.state && view.state->type != Roo::Value::Type::NIL) return view.state;
      return view.initial_state;
    }

    if (StateBindings::is_binding(binding))
    {
      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return parent;
      return Roo::Dict::get_property_path(parent, path);
    }

    auto result = (view.state && view.state->type != Roo::Value::Type::NIL)
                    ? Roo::Dict::shallow_copy(view.state)
                    : Roo::map({});
    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto value = Roo::Dict::get_property(binding, key);
      if (StateBindings::contains_binding(value))
      {
        Roo::Dict::set_property(
          result,
          key,
          StateBindings::resolve_value(parent, Roo::Dict::get_property(result, key), value));
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

    if (StateBindings::is_binding(binding))
    {
      if (!Pixils::Runtime::bind_state_writable(binding)) return parent;

      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return child_state;
      return Roo::Dict::assoc_in(parent, path, child_state);
    }

    auto result = StateBindings::merge_pass(parent, binding, child_state, true);
    result = StateBindings::merge_pass(result, binding, child_state, false);
    return result;
  }

  const Roo::sptr_val_v& bind_state_path(const Roo::sptr_val& value)
  {
    return Roo::obj<Runtime::BindState>(*value).path;
  }

  bool bind_state_writable(const Roo::sptr_val& value)
  {
    return Roo::obj<Runtime::BindState>(*value).writable;
  }

  StateBinding parse_state_binding(const Roo::sptr_val& state_value)
  {
    if (StateBindings::is_binding(state_value))
    {
      return {state_value, Roo::Constant::NIL};
    }

    if (!state_value || state_value->type == Roo::Value::Type::NIL) return {};

    Roo::sptr_val literal = Roo::map({});
    bool has_binding = false;
    for (const auto& key : Roo::Dict::map_sptr_keys(state_value))
    {
      auto value = Roo::Dict::get_property(state_value, key);
      if (StateBindings::contains_binding(value))
      {
        has_binding = true;
        if (value && value->type == Roo::Value::Type::MAP)
        {
          Roo::Dict::set_property(literal, key, StateBindings::literal_value(value));
        }
      }
      else
      {
        Roo::Dict::set_property(literal, key, value);
      }
    }

    if (has_binding) return {state_value, literal};

    return {Roo::Constant::NIL, state_value};
  }

} // namespace Pixils::Runtime
