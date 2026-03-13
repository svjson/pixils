
#include <pixils/arg_collector.h>

#include <lisple/exception.h>
#include <lisple/form.h>
#include <lisple/impl.h>
#include <lisple/type.h>

namespace Lisple
{
  class Context;
}

namespace Pixils
{
  ArgCollector::ArgCollector(const std::string& function_name, const ArgKeyMap& required,
                             const ArgKeyMap& optional)
      : function_name(function_name)
      , required_keys(required)
      , optional_keys(optional)
  {
  }

  str_key_map_t ArgCollector::collect_keys(Lisple::Context& ctx, Lisple::Object& obj_map)
  {
    return collect_keys(obj_map, &ctx);
  }

  str_key_map_t ArgCollector::collect_keys(Lisple::Object& obj_map, Lisple::Context* ctx)
  {
    return collect_keys(obj_map.as<Lisple::Map>(), ctx);
  }

  str_key_map_t ArgCollector::collect_keys(Lisple::Map& arg_map, Lisple::Context* ctx)
  {
    str_key_map_t verified_keys;

    for (auto& [k, type] : required_keys)
    {
      if (arg_map.has_key(k))
      {
        auto val = arg_map.get_sptr_property(k);
        if (type->is_type_of(*val) && *val != *Lisple::NIL)
        {
          verified_keys.emplace(k.value, val);
        }
        else if (ctx)
        {
          Lisple::CoercionResult coercion = type->coerce(*ctx, val);
          if (coercion.success)
          {
            verified_keys.emplace(k.value, coercion.result);
          }
          else
          {
            throw Lisple::InvocationException("Invalid argument invoking " + function_name +
                                              ". Incorrect type of value for key " + k.to_string() +
                                              " in " + arg_map.to_string() +
                                              " and coercion attempt failed. Expected: " +
                                              type->to_string() + ", Got: " + val->to_string());
          }
        }
        else
        {
          throw Lisple::InvocationException(
              "Invalid argument invoking " + function_name + ". Incorrect type of value for key " +
              k.to_string() + " in " + arg_map.to_string() +
              " and coercion is not possible. Expecting: " + type->to_string());
        }
      }
      else
      {
        throw Lisple::InvocationException("Invalid argument invoking " + function_name +
                                          ". Required key " + k.to_string() +
                                          " missing in argument: " + arg_map.to_string());
      }
    }

    for (auto& [k, type] : optional_keys)
    {
      if (arg_map.has_key(k))
      {
        auto val = arg_map.get_sptr_property(k);
        if (type->is_type_of(*val))
        {
          if (*val != *Lisple::NIL)
          {
            verified_keys.emplace(k.value, val);
          }
        }
        else if (*val == *Lisple::NIL)
        {
          continue;
        }
        else if (ctx)
        {
          Lisple::CoercionResult coercion = type->coerce(*ctx, val);
          if (coercion.success)
          {
            verified_keys.emplace(k.value, coercion.result);
          }
          else
          {
            throw Lisple::InvocationException("Invalid argument invoking " + function_name +
                                              ". Incorrect type of value for key " + k.to_string() +
                                              " in " + arg_map.to_string() +
                                              " and coercion attempt failed. Expected: " +
                                              type->to_string() + ", Got: " + val->to_string());
          }
        }
        else
        {
          std::string type_name;
          switch (val->get_type())
          {
          case Lisple::Form::MAP:
            type_name = "{}";
            break;
          case Lisple::Form::HOST_OBJECT:
            type_name = "HostObject";
            break;
          default:
            type_name = "<unspecified>";
          }

          throw Lisple::InvocationException(
              "Invalid argument invoking " + function_name + ". Incorrect type of value for key " +
              k.to_string() + " in " + arg_map.to_string() + ". Expecting: " + type->to_string() +
              ". Coercion from " + type_name +
              " could not be attempted, because runtime ctx is not available.");
        }
      }
    }

    for (auto& k : arg_map.key_ptrs())
    {
      if (!Lisple::Type::KEY.is_type_of(*k) ||
          (required_keys.count(k->as<Lisple::Key>().value) == 0 &&
           optional_keys.count(k->as<Lisple::Key>().value) == 0))
      {
        throw Lisple::InvocationException("Invalid argument invoking " + function_name +
                                          ". Unexpected key " + k->to_string() + " in " +
                                          arg_map.to_string());
      }
    }

    return verified_keys;
  }

  bool ArgCollector::is_type(str_key_map_t& args, const Lisple::Key& key,
                             const Lisple::TypeRef* type_ref)
  {
    if (args.count(key.value) != 1)
      return false;

    return type_ref->is_type_of(*args.at(key.value));
  }

  char ArgCollector::char_value(str_key_map_t& args, const Lisple::Key& key, char default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }
    return args.at(key.value)->as<Lisple::Char>().value;
  }

  float ArgCollector::float_value(str_key_map_t& args, const Lisple::Key& key, float default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }
    return Lisple::float_val(*args.at(key.value));
  }

  int ArgCollector::int_value(str_key_map_t& args, const Lisple::Key& key, int default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }
    return Lisple::int_val(*args.at(key.value));
  }

  short ArgCollector::short_value(str_key_map_t& args, const Lisple::Key& key, short default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }
    return Lisple::short_val(*args.at(key.value));
  }

  unsigned short ArgCollector::ushort_value(str_key_map_t& args, const Lisple::Key& key,
                                            unsigned short default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }
    return Lisple::ushort_val(*args.at(key.value));
  }

  uint8_t ArgCollector::uint8_value(str_key_map_t& args, const Lisple::Key& key,
                                    uint8_t default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }

    int value = args.at(key.value)->as<Lisple::Number>().value;
    if (value < 0 || value > 255)
    {
      throw Lisple::TypeError("Invalid value " + std::to_string(value) +
                              " for 8-bit unsigned integer associated with key " + key.to_string());
    }
    return static_cast<uint8_t>(value);
  }

  const std::string ArgCollector::str_value(str_key_map_t& args, const Lisple::Key& key,
                                            const std::string& default_value)
  {
    if (!args.count(key.value))
    {
      return default_value;
    }

    const Lisple::Object& obj = *args.at(key.value);

    if (obj == *Lisple::NIL)
      return default_value;

    if (Lisple::Type::STRING.is_type_of(obj) || Lisple::Type::KEY.is_type_of(obj) ||
        Lisple::Type::SYMBOL.is_type_of(obj) || Lisple::Type::WORD.is_type_of(obj))
    {
      return obj.as<Lisple::Value<std::string>>().value;
    }
    else if (Lisple::Type::NUMBER.is_type_of(obj))
    {
      return obj.to_string();
    }

    return default_value;
  }

  bool ArgCollector::bool_value(str_key_map_t& args, const Lisple::Key& key, bool default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }

    return args.at(key.value)->as<Lisple::Boolean>().value;
  }

  Lisple::Map& ArgCollector::lmap_value(str_key_map_t& args, const Lisple::Key& key)
  {
    return args.at(key.value)->as<Lisple::Map>();
  }

  Lisple::Array& ArgCollector::array_value(str_key_map_t& args, const Lisple::Key& key)
  {
    return args.at(key.value)->as<Lisple::Array>();
  }

  Lisple::Key& ArgCollector::key_value(str_key_map_t& args, const Lisple::Key& key)
  {
    return args.at(key.value)->as<Lisple::Key>();
  }

  Lisple::Key& ArgCollector::key_value(str_key_map_t& args, const Lisple::Key& key,
                                       Lisple::Key& default_value)
  {
    if (args.count(key.value))
    {
      return key_value(args, key);
    }
    return default_value;
  }

  std::vector<uint8_t> ArgCollector::unbox_uint8_array(str_key_map_t& args, const Lisple::Key& key,
                                                       const std::vector<uint8_t> default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }

    std::vector<uint8_t> result;
    for (auto& lmnt : args.at(key.value)->get_children())
    {
      result.push_back(Lisple::int_val(*lmnt));
    }
    return result;
  }

  std::vector<int> ArgCollector::unbox_int_array(str_key_map_t& args, const Lisple::Key& key,
                                                 const std::vector<int> default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }

    std::vector<int> result;
    for (auto& lmnt : args.at(key.value)->get_children())
    {
      result.push_back(Lisple::int_val(*lmnt));
    }
    return result;
  }

  std::vector<std::string>
  ArgCollector::unbox_string_array(str_key_map_t& args, const Lisple::Key& key,
                                   const std::vector<std::string>& default_value)
  {
    if (!args.count(key.value) || *args.at(key.value) == *Lisple::NIL)
    {
      return default_value;
    }

    std::vector<std::string> result;
    for (auto& lmnt : args.at(key.value)->get_children())
    {
      result.push_back(Lisple::str_val(*lmnt));
    }
    return result;
  }

  std::optional<std::string> ArgCollector::optional_string(str_key_map_t& args,
                                                           const Lisple::Key& key)
  {
    if (args.count(key.value))
    {
      return Lisple::str_val(*args.at(key.value));
    }
    return std::nullopt;
  }

  std::optional<int> ArgCollector::optional_int(str_key_map_t& args, const Lisple::Key& key)
  {
    if (args.count(key.value))
    {
      return Lisple::int_val(*args.at(key.value));
    }
    return std::nullopt;
  }

  std::unique_ptr<ArgCollector> ArgCollector::make(const std::string& function_name,
                                                   const ArgKeyMap& required_keys,
                                                   const ArgKeyMap& optional_keys)
  {
    return std::make_unique<ArgCollector>(function_name, required_keys, optional_keys);
  }

} // namespace Pixils
