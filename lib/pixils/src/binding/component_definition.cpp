#include "pixils/binding/component_definition.h"

#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/runtime/mode.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exception.h>
#include <roo/host/object.h>
#include <roo/host/schema.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace
  {
    bool is_component_definition_key(const std::string& key)
    {
      return key == "init-ui" || key == "update-ui" || key == "after-layout-ui" ||
             key == "ui/state-keys" || key == "ui/models";
    }

    Roo::sptr_val mode_definition_map(const Roo::sptr_val& definition_map)
    {
      auto mode_map = Roo::map({});
      for (auto& key : Roo::Dict::keys(*definition_map))
      {
        if (key->str() == "extend" || is_component_definition_key(key->str()))
        {
          continue;
        }
        Roo::Dict::set_property(mode_map, key, Roo::Dict::get_property(definition_map, key));
      }
      return mode_map;
    }

    Roo::sptr_val eval_hook(Roo::Context& ctx, const Roo::sptr_val& val)
    {
      if (val && val->type == Roo::Value::Type::LIST)
      {
        return ctx.eval(val->to_string());
      }
      return val ? val : Roo::Constant::NIL;
    }

    std::vector<std::string> parse_ui_state_keys(const Roo::sptr_val& keys_val)
    {
      std::vector<std::string> keys;
      if (!keys_val || keys_val->type == Roo::Value::Type::NIL) return keys;
      if (keys_val->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("Component :ui/state-keys must be a vector");
      }

      for (const auto& key : Roo::get_children(*keys_val))
      {
        if (!key || key->type != Roo::Value::Type::KEYWORD)
        {
          throw Roo::TypeError("Component :ui/state-keys entries must be keywords");
        }
        keys.push_back(key->str());
      }
      return keys;
    }

    std::vector<std::string> parse_model_keys(const Roo::sptr_val& keys_val,
                                              const std::string& field)
    {
      if (!keys_val || keys_val->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("UI state model :" + field + " must be a vector");
      }

      std::vector<std::string> keys;
      for (const auto& key : Roo::get_children(*keys_val))
      {
        if (!key || key->type != Roo::Value::Type::KEYWORD)
        {
          throw Roo::TypeError("UI state model :" + field + " entries must be keywords");
        }
        keys.push_back(key->str());
      }
      return keys;
    }

    std::vector<Runtime::UIStateModel> parse_ui_state_models(
      const Roo::sptr_val& models_val,
      std::vector<std::string> owned_keys)
    {
      if (!models_val || models_val->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("Component :ui/models must be a vector");
      }

      std::vector<Runtime::UIStateModel> models;
      for (const auto& model_val : Roo::get_children(*models_val))
      {
        if (!model_val || model_val->type != Roo::Value::Type::MAP)
        {
          throw Roo::TypeError("Component :ui/models entries must be maps");
        }

        auto owned = Roo::Dict::get_property(model_val, Roo::keyword("owns"));
        auto dependencies = Roo::Dict::get_property(model_val, Roo::keyword("depends-on"));
        auto transition = Roo::Dict::get_property(model_val, Roo::keyword("transition"));
        auto change_event = Roo::Dict::get_property(model_val, Roo::keyword("change-event"));
        if (!transition || transition->type != Roo::Value::Type::FUNCTION)
        {
          throw Roo::TypeError("UI state model :transition must be a function");
        }
        if (change_event && change_event->type != Roo::Value::Type::NIL &&
            change_event->type != Roo::Value::Type::KEYWORD)
        {
          throw Roo::TypeError("UI state model :change-event must be a keyword");
        }

        Runtime::UIStateModel model;
        model.owned_keys = parse_model_keys(owned, "owns");
        model.dependency_keys = parse_model_keys(dependencies, "depends-on");
        model.transition = transition;
        model.change_event = change_event ? change_event : Roo::Constant::NIL;
        if (model.owned_keys.empty())
        {
          throw Roo::TypeError("UI state model :owns cannot be empty");
        }
        for (const auto& key : model.owned_keys)
        {
          if (std::find(owned_keys.begin(), owned_keys.end(), key) != owned_keys.end())
          {
            throw Roo::TypeError("Component UI state models cannot both own :" + key);
          }
          owned_keys.push_back(key);
        }
        models.push_back(std::move(model));
      }
      return models;
    }

  } // namespace

  void reject_component_definition_fields(const Roo::sptr_val& definition_map,
                                          const std::string& context)
  {
    if (!definition_map || definition_map->type != Roo::Value::Type::MAP)
    {
      return;
    }

    for (auto& key : Roo::Dict::keys(*definition_map))
    {
      if (is_component_definition_key(key->str()))
      {
        throw Roo::TypeError(context + " cannot use component UI state fields");
      }
    }
  }

  bool is_view_definition_registry_value(const Roo::sptr_val& value)
  {
    return value &&
           (HostType::MODE.is_type_of(*value) || HostType::COMPONENT.is_type_of(*value));
  }

  Runtime::ViewDefinition& definition_from_registry_value(const Roo::sptr_val& value,
                                                          const std::string& context)
  {
    if (HostType::MODE.is_type_of(*value))
    {
      return Roo::obj<Runtime::Mode>(*value);
    }
    if (HostType::COMPONENT.is_type_of(*value))
    {
      return Roo::obj<Runtime::Component>(*value);
    }
    throw Roo::InvocationException(context + " resolved to non-mode value");
  }

  Runtime::Component* component_from_registry_value(const Roo::sptr_val& value)
  {
    if (value && HostType::COMPONENT.is_type_of(*value))
    {
      return &Roo::obj<Runtime::Component>(*value);
    }
    return nullptr;
  }

  Runtime::Component build_component_from_definition(Roo::Context& ctx,
                                                     const Roo::sptr_val& definition_map)
  {
    static Roo::MapSchema component_schema({},
                                           {{"extend", &Roo::Type::SYMBOL_VALUE},
                                            {"init-ui", &Roo::Type::ANY},
                                            {"update-ui", &Roo::Type::ANY},
                                            {"after-layout-ui", &Roo::Type::ANY},
                                            {"ui/state-keys", &Roo::Type::VECTOR},
                                            {"ui/models", &Roo::Type::VECTOR}});

    auto opts = component_schema.bind(ctx, *definition_map);
    if (Roo::Dict::contains_key(*definition_map, "compose"))
    {
      throw Roo::TypeError("defcomponent cannot use mode composition");
    }

    Runtime::Component component;
    Runtime::Mode mode_base;
    if (opts.contains("extend"))
    {
      auto extends_name = opts.str("extend", "");
      auto components = ctx.lookup(ID__PIXILS__COMPONENTS);
      auto base_val = Roo::Dict::get_property(components, Roo::symbol(extends_name));
      if (!base_val || base_val->type == Roo::Value::Type::NIL)
        throw Roo::InvocationException("defcomponent :extends - unknown base component '" +
                                       extends_name + "'");

      auto base_component = component_from_registry_value(base_val);
      if (base_component)
      {
        component = *base_component;
        static_cast<Runtime::ViewDefinition&>(mode_base) = *base_component;
      }
      else
      {
        throw Roo::InvocationException("defcomponent :extends - base '" + extends_name +
                                       "' resolved to non-component value");
      }
    }

    auto parsed_mode =
      build_mode_from_definition(ctx, mode_definition_map(definition_map), &mode_base);
    static_cast<Runtime::ViewDefinition&>(component) = parsed_mode;

    auto apply_hook = [&](Roo::sptr_val& field, const char* key)
    {
      if (opts.contains(key))
      {
        field = eval_hook(ctx, opts.val(key));
      }
    };

    apply_hook(component.init_ui, "init-ui");
    apply_hook(component.update_ui, "update-ui");
    apply_hook(component.after_layout_ui, "after-layout-ui");
    if (opts.contains("ui/state-keys"))
    {
      component.ui_state_keys = parse_ui_state_keys(opts.val("ui/state-keys"));
    }
    if (opts.contains("ui/models"))
    {
      std::vector<std::string> inherited_owned_keys;
      for (const auto& model : component.ui_state_models)
      {
        inherited_owned_keys.insert(inherited_owned_keys.end(),
                                    model.owned_keys.begin(),
                                    model.owned_keys.end());
      }
      auto models = parse_ui_state_models(opts.val("ui/models"), inherited_owned_keys);
      component.ui_state_models.insert(component.ui_state_models.end(),
                                       models.begin(),
                                       models.end());
    }

    return component;
  }

} // namespace Pixils::Script
