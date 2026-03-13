
#ifndef __ARG_COLLECTOR_H_
#define __ARG_COLLECTOR_H_

#include <cstdint>
#include <lisple/context.h>
#include <lisple/exception.h>
#include <lisple/form.h>
#include <lisple/host.h>
#include <lisple/impl.h>
#include <lisple/type.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace Pixils
{
  typedef std::map<Lisple::Key, const Lisple::TypeRef*> ArgKeyMap;
  typedef std::map<const std::string, Lisple::sptr_sobject> str_key_map_t;

  /*!
   * @brief Utility functions and rulesets for validating and coercing
   * script funtion arguments.
   */
  class ArgCollector
  {
  private:
    /*!
     * @brief The name of the function that this instance describes.
     */
    const std::string function_name;
    /*!
     * @brief The keys and expected types that are required when invoking the
     * function referred to by @ref function_name.
     *
     * If any of the specified keys are missing from the argument map or is
     * of the wrong type when invoking @ref ArgCollector::collect_keys, the
     * operation will fail.
     */
    ArgKeyMap required_keys;
    /*!
     * @brief The keys and expected types that are optional when invoking the
     * function referred to by @ref function_name.
     *
     * No error will be raised if any of these keys are missing from the
     * argument map when invoking @ref ArgCollector::collect_keys, the
     * operation will fail, but they must be of the specified type.
     */
    ArgKeyMap optional_keys;

  public:
    ArgCollector(const std::string& function_name, const ArgKeyMap& required_keys,
                 const ArgKeyMap& optional_keys = {});

    /*!
     * @brief Validate and collect all keys of a map.
     */
    str_key_map_t collect_keys(Lisple::Context& ctx, Lisple::Object& obj_map);
    /*!
     * @brief Validate and collect all keys of a map.
     */
    str_key_map_t collect_keys(Lisple::Object& obj_map, Lisple::Context* = nullptr);
    /*!
     * @brief Validate and collect all keys of a map.
     */
    str_key_map_t collect_keys(Lisple::Map& args, Lisple::Context* = nullptr);

    static bool is_type(str_key_map_t& args, const Lisple::Key& key,
                        const Lisple::TypeRef* type_ref);

    /*!
     * @brief Get the value of the property @a key of @a args as char, or the
     * default value of @a default_value, if the property is missing.
     */
    static char char_value(str_key_map_t& args, const Lisple::Key& key,
                           char default_value = '\0');
    /*!
     * @brief Get the value of the property @a key of @a args as int, or the
     * default value of @a default_value, if the property is missing.
     */
    static float float_value(str_key_map_t& args, const Lisple::Key& key,
                             float default_value = 0.0);
    /*!
     * @brief Get the value of the property @a key of @a args as int, or the
     * default value of @a default_value, if the property is missing.
     */
    static int int_value(str_key_map_t& args, const Lisple::Key& key, int default_value = 0);
    /*!
     * @brief Get the value of the property @a key of @a args as unsigned short,
     * or the default value of @a default_value, if the property is missing.
     */
    static unsigned short ushort_value(str_key_map_t& args, const Lisple::Key& key,
                                       unsigned short default_value = 0);
    /*!
     * @brief Get the value of the property @a key of @a args as short, or the
     * default value of @a default_value, if the property is missing.
     */
    static short short_value(str_key_map_t& args, const Lisple::Key& key,
                             short default_value = 0);
    /*!
     * @brief Get the value of the property @a key of @a args as uint8, or the
     * default value of @a default_value, if the property is missing.
     */
    static uint8_t uint8_value(str_key_map_t& args, const Lisple::Key& key,
                               uint8_t default_value = 0xff);

    /*!
     * @brief Get the value of the property @a key of @a args as std::string, or
     * the default value of @a default_value, if the property is missing.
     */
    static const std::string
    str_value(str_key_map_t& args, const Lisple::Key& key,
              const std::string& default_value = Lisple::EMPTY_STRING);

    /*!
     * @brief Get the value of the property @a key of @a args as bool, or the
     * default value of @a default_value, if the property is missing.
     */
    static bool bool_value(str_key_map_t& args, const Lisple::Key& key,
                           bool default_value = false);
    /*!
     * @brief Get the value of the property @a key of @a args as
     * std::optional<bool>, which will have the value of std::nullopt if the
     * property is missing.
     */
    static std::optional<std::string> optional_string(str_key_map_t& args,
                                                      const Lisple::Key& key);

    /*!
     * @brief Get the value of the property @a key of @a args as
     * std::optional<int>, which will have the value of std::nullopt if the
     * property is missing.
     */
    static std::optional<int> optional_int(str_key_map_t& args, const Lisple::Key& key);

    /*!
     * @brief Get the Lisple::Array stored under the property @a key of @a args.
     */
    static Lisple::Array& array_value(str_key_map_t&, const Lisple::Key& key);
    /*!
     * @brief Get the Lisple::Map stored under the property @a key of @a args.
     */
    static Lisple::Map& lmap_value(str_key_map_t& args, const Lisple::Key& key);
    /*!
     * @brief Get the Lisple::Key stored under the property @a key of @a args.
     */
    static Lisple::Key& key_value(str_key_map_t& rags, const Lisple::Key& key);
    /*!
     * @brief Get the Lisple::Key stored under the property @a key of @a args,
     * or @a default_value if the key is missing or nil.
     */
    static Lisple::Key& key_value(str_key_map_t& rags, const Lisple::Key& key,
                                  Lisple::Key& default_value);

    static std::vector<uint8_t> unbox_uint8_array(str_key_map_t& args, const Lisple::Key& key,
                                                  const std::vector<uint8_t> default_val = {});
    static std::vector<int> unbox_int_array(str_key_map_t& args, const Lisple::Key& key,
                                            const std::vector<int> default_val = {});
    static std::vector<std::string>
    unbox_string_array(str_key_map_t& args, const Lisple::Key& key,
                       const std::vector<std::string>& default_val = {});

    template <typename T>
    static std::shared_ptr<T> sptr_value(str_key_map_t& keys, const Lisple::Key& key)
    {
      return std::dynamic_pointer_cast<T>(keys.at(key.value));
    }

    template <typename T> static T& get(str_key_map_t& keys, const Lisple::Key& key)
    {
      return keys.at(key.value)->as<Lisple::HostObject<T>>().get_object();
    }

    /*!
     * @brief General purpose function for unwrapping a Lisple sequence to a
     * vector of objects, possibly coercing and converting raw lisple data
     * structures to the specified native type.
     */
    template <typename VT>
    static std::vector<VT> vector(Lisple::Context& ctx, str_key_map_t& keys,
                                  const Lisple::Key& key, const Lisple::HostTypeRef* type_ref,
                                  std::vector<VT> default_value = {})
    {
      if (keys.count(key.value))
      {
        std::vector<VT> result;
        Lisple::Seq& seq = keys.at(key.value)->as<Lisple::Seq>();
        result.reserve(seq.size());

        for (auto& obj : seq.get_children())
        {
          result.push_back(coerce_host_object(ctx, obj, type_ref)
                               ->as<Lisple::HostObject<VT>>()
                               .get_object());
        }

        return result;
      }

      return default_value;
    }

    template <typename VT, typename HT>
    static std::vector<VT> unbox_host_object_array_to_objects(
        Lisple::Context& ctx, str_key_map_t& keys, const Lisple::Key& key,
        const Lisple::HostTypeRef* type_ref, const std::string& make_fn_name = "")
    {
      std::vector<VT> result;

      if (keys.count(key.value))
      {
        Lisple::Array& array = ArgCollector::array_value(keys, key);
        for (auto& obj : array.get_children())
        {
          result.push_back(
              coerce_host_object(ctx, obj, type_ref, make_fn_name)->as<HT>().get_object());
        }
      }

      return result;
    }

    template <typename VT>
    static std::optional<VT>
    optional_host_object(Lisple::Context& ctx, str_key_map_t& keys, const Lisple::Key& key,
                         const Lisple::HostTypeRef* type_ref,
                         const std::optional<VT> default_value = std::nullopt)
    {
      if (keys.count(key.value))
      {
        if (*keys.at(key.value) == *Lisple::NIL)
          return default_value;
        return coerce_host_object(ctx, keys.at(key.value), type_ref)
            ->as<Lisple::HostObject<VT>>()
            .get_object();
      }

      return default_value;
    }

    template <typename VT>
    static std::optional<VT>
    optional_host_object(str_key_map_t& keys, const Lisple::Key& key,
                         const std::optional<VT> default_value = std::nullopt)
    {
      if (keys.count(key.value))
      {
        if (*keys.at(key.value) == *Lisple::NIL)
          return default_value;
        return keys.at(key.value)->as<Lisple::HostObject<VT>>().get_object();
      }

      return default_value;
    }

    template <typename VT>
    static VT coerce_host_object(Lisple::Context& ctx, str_key_map_t keys,
                                 const Lisple::Key& key, const Lisple::HostTypeRef* type_ref,
                                 const std::string& make_fn_name = "")
    {
      return coerce_host_object(ctx, keys.at(key.value), type_ref, make_fn_name)
          ->as<Lisple::HostObject<VT>>()
          .get_object();
    }

    static Lisple::sptr_sobject coerce_host_object(Lisple::Context& ctx,
                                                   Lisple::sptr_sobject& obj,
                                                   const Lisple::HostTypeRef* type_ref,
                                                   const std::string& make_fn_name = "")
    {
      if (type_ref->is_type_of(*obj))
      {
        return obj;
      }
      else if (Lisple::Type::MAP.is_type_of(*obj) &&
               (!make_fn_name.empty() || type_ref->make_fn.has_value()))
      {
        return ctx.call(type_ref->make_fn ? *type_ref->make_fn : make_fn_name, obj);
      }
      else
      {
        Lisple::CoercionResult result = type_ref->coerce(ctx, obj);
        if (result.success)
        {
          return result.result;
        }
        throw Lisple::TypeError("Cannot be interpreted as " + type_ref->to_string() + ": " +
                                obj->to_string());
      }
    }

    template <typename HT>
    static HT* get_host_object(str_key_map_t keys, const Lisple::Key& key)
    {
      if (!keys.count(key.value))
        return nullptr;
      return &keys.at(key.value)->as<Lisple::HostObject<HT>>().get_object();
    }

    template <typename VT, typename LT>
    static std::vector<VT> unbox_array(str_key_map_t& args, const Lisple::Key& key)
    {
      Lisple::Array& adapter_array = args.at(key.value)->as<Lisple::Array>();
      std::vector<VT> result;
      for (auto& obj : adapter_array.get_children())
      {
        if constexpr (std::is_arithmetic<VT>::value || std::is_same<VT, std::string>::value)
        {
          result.push_back(static_cast<VT>(obj->as<LT>().value));
        }
        else
        {
          result.push_back(obj->as<LT>().get_object());
        }
      }
      return result;
    }

    template <typename T, typename A>
    static std::vector<T*> unbox_host_object_array(str_key_map_t& args, const Lisple::Key& key)
    {
      Lisple::Array& adapter_array = args.at(key.value)->as<Lisple::Array>();
      std::vector<T*> result;
      for (auto& obj : adapter_array.get_children())
      {
        result.push_back(&obj->as<A>().get_object());
      }
      return result;
    }

    static std::unique_ptr<ArgCollector> make(const std::string& function_name,
                                              const ArgKeyMap& required_keys,
                                              const ArgKeyMap& optional_keys = {});
  };
} // namespace Pixils

#endif
