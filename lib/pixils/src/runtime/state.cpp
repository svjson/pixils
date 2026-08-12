
#include "pixils/runtime/state.h"

#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/binding/ui/ui_namespace.h>
#include <pixils/runtime/component.h>
#include <pixils/runtime/view.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exception.h>
#include <roo/host/object.h>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>

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

    Roo::sptr_val child_property(const Roo::sptr_val& child_state, const Roo::sptr_val& key)
    {
      if (!child_state || child_state->type != Roo::Value::Type::MAP)
      {
        return Roo::Constant::NIL;
      }
      return Roo::Dict::get_property(child_state, key);
    }

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

    Roo::sptr_val merge_state_binding_pass(Roo::sptr_val result,
                                           const Roo::sptr_val& binding,
                                           const Roo::sptr_val& child_state,
                                           bool whole_parent_bindings)
    {
      if (is_bind_state(binding))
      {
        if (!Pixils::Runtime::bind_state_writable(binding)) return result;

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
        auto val = Roo::Dict::get_property(binding, key);
        if (contains_state_binding(val))
        {
          result = merge_state_binding_pass(result,
                                            val,
                                            child_property(child_state, key),
                                            whole_parent_bindings);
        }
      }
      return result;
    }

    bool state_values_equal(const Roo::sptr_val& lhs, const Roo::sptr_val& rhs)
    {
      if (lhs == rhs) return true;
      if (!lhs || !rhs || lhs->type != rhs->type) return false;
      return *lhs == *rhs;
    }

  } // namespace

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

    if (is_bind_state(binding))
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
      if (!Pixils::Runtime::bind_state_writable(binding)) return parent;

      const auto& path = Pixils::Runtime::bind_state_path(binding);
      if (path.empty()) return child_state;
      return Roo::Dict::assoc_in(parent, path, child_state);
    }

    auto result = merge_state_binding_pass(parent, binding, child_state, true);
    result = merge_state_binding_pass(result, binding, child_state, false);
    return result;
  }

  Roo::sptr_val extract_component_ui_state(const Roo::sptr_val& parent,
                                           const Pixils::Runtime::View& view)
  {
    const auto& binding = view.component_state_binding;

    if (!view.component || !binding || binding->type == Roo::Value::Type::NIL)
    {
      return view.ui_state;
    }

    auto result = view.ui_state && view.ui_state->type == Roo::Value::Type::MAP
                    ? Roo::Dict::shallow_copy(view.ui_state)
                    : Roo::map({});

    if (is_bind_state(binding))
    {
      const auto& path = Pixils::Runtime::bind_state_path(binding);
      auto bound_value = path.empty() ? parent : Roo::Dict::get_property_path(parent, path);
      auto declared = declared_component_ui_state(view, bound_value);
      for (const auto& key : Roo::Dict::map_sptr_keys(declared))
      {
        Roo::Dict::set_property(result, key, Roo::Dict::get_property(declared, key));
      }
      return result;
    }

    if (binding->type != Roo::Value::Type::MAP)
    {
      return result;
    }

    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto val = Roo::Dict::get_property(binding, key);
      if (declared_component_ui_key(view, key) && contains_state_binding(val))
      {
        Roo::Dict::set_property(
          result,
          key,
          resolve_state_binding_value(parent, Roo::Dict::get_property(result, key), val));
      }
    }
    return result;
  }

  Roo::sptr_val merge_component_ui_state(const Roo::sptr_val& parent,
                                         const Pixils::Runtime::View& view,
                                         const Roo::sptr_val& ui_state)
  {
    const auto& binding = view.component_state_binding;

    if (!view.component || !binding || binding->type == Roo::Value::Type::NIL)
    {
      return parent;
    }

    if (is_bind_state(binding))
    {
      if (!Pixils::Runtime::bind_state_writable(binding)) return parent;

      const auto& path = Pixils::Runtime::bind_state_path(binding);
      auto value = declared_component_ui_state(view, ui_state);
      if (path.empty()) return value;
      return Roo::Dict::assoc_in(parent, path, value);
    }

    if (binding->type != Roo::Value::Type::MAP)
    {
      return parent;
    }

    auto result = parent;
    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto val = Roo::Dict::get_property(binding, key);
      if (declared_component_ui_key(view, key) && contains_state_binding(val))
      {
        result = merge_state_binding_pass(result, val, ui_property(ui_state, key), true);
      }
    }
    for (const auto& key : Roo::Dict::map_sptr_keys(binding))
    {
      auto val = Roo::Dict::get_property(binding, key);
      if (declared_component_ui_key(view, key) && contains_state_binding(val))
      {
        result = merge_state_binding_pass(result, val, ui_property(ui_state, key), false);
      }
    }
    return result;
  }

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
    }

    return view.set_ui_state_if_changed(next);
  }

  bool transition_component_ui_state(Pixils::Runtime::View& view,
                                     const Roo::sptr_val& candidate,
                                     Roo::Runtime& runtime,
                                     bool initializing)
  {
    Roo::Context ctx(runtime);
    return transition_component_ui_state(view, candidate, ctx, initializing);
  }

  const Roo::sptr_val_v& bind_state_path(const Roo::sptr_val& val)
  {
    return Roo::obj<Runtime::BindState>(*val).path;
  }

  bool bind_state_writable(const Roo::sptr_val& val)
  {
    return Roo::obj<Runtime::BindState>(*val).writable;
  }

  bool state_binding_controls_key(const Pixils::Runtime::View& view, const std::string& key)
  {
    const auto& binding = view.state_binding;
    if (!binding || binding->type == Roo::Value::Type::NIL) return false;
    if (is_bind_state(binding)) return true;
    if (binding->type != Roo::Value::Type::MAP) return false;

    auto value = Roo::Dict::get_property(binding, Roo::keyword(key));
    return contains_state_binding(value);
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
