#include "pixils/binding/ui/style/theme_definition.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/ui/style/style_host_type.h>
#include <pixils/ui/theme.h>

#include <lisple/context.h>
#include <lisple/exception.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>

#include <set>

namespace Pixils::Script
{
  const std::string THEME_VAR_MARKER_KEY = "__pixils-theme-var";

  namespace
  {
    std::string variant_key_name(const Lisple::sptr_val& value, const std::string& context)
    {
      if (!value || (value->type != Lisple::Value::Type::KEYWORD &&
                     value->type != Lisple::Value::Type::SYMBOL))
      {
        throw Lisple::TypeError(context + " keys must be keywords or symbols");
      }
      return value->str();
    }

    std::string parse_default_variant(const Lisple::sptr_val& value)
    {
      if (!value || value->type == Lisple::Value::Type::NIL) return "";
      if (value->type != Lisple::Value::Type::KEYWORD &&
          value->type != Lisple::Value::Type::SYMBOL)
      {
        throw Lisple::TypeError("Theme :default-variant must be a keyword or symbol");
      }
      return value->str();
    }

    bool is_theme_var_ref(const Lisple::sptr_val& value)
    {
      if (!value || value->type != Lisple::Value::Type::MAP) return false;
      auto var_ref =
        Lisple::Dict::get_property(value, Lisple::keyword(THEME_VAR_MARKER_KEY));
      return var_ref && var_ref->type != Lisple::Value::Type::NIL;
    }

    std::string theme_var_ref_name(const Lisple::sptr_val& value)
    {
      auto var_ref =
        Lisple::Dict::get_property(value, Lisple::keyword(THEME_VAR_MARKER_KEY));
      if (!var_ref || (var_ref->type != Lisple::Value::Type::KEYWORD &&
                       var_ref->type != Lisple::Value::Type::SYMBOL))
      {
        throw Lisple::TypeError("Theme var references must use keyword or symbol names");
      }
      return var_ref->str();
    }

    std::map<std::string, std::map<std::string, Lisple::sptr_val>> parse_theme_vars(
      const Lisple::sptr_val& value)
    {
      std::map<std::string, std::map<std::string, Lisple::sptr_val>> vars;
      if (!value || value->type == Lisple::Value::Type::NIL) return vars;
      if (value->type != Lisple::Value::Type::MAP)
      {
        throw Lisple::TypeError("Theme :vars must be a map");
      }

      for (auto& variant_key : Lisple::Dict::keys(*value))
      {
        auto variant_name = variant_key_name(variant_key, "Theme :vars");
        auto variant_vars = Lisple::Dict::get_property(value, variant_key);
        if (!variant_vars || variant_vars->type != Lisple::Value::Type::MAP)
        {
          throw Lisple::TypeError("Theme :vars entries must be maps");
        }

        auto& out = vars[variant_name];
        for (auto& var_key : Lisple::Dict::keys(*variant_vars))
        {
          auto var_name = variant_key_name(var_key, "Theme variable");
          out[var_name] = Lisple::Dict::get_property(variant_vars, var_key);
        }
      }

      return vars;
    }

    Lisple::sptr_val lookup_theme_var_internal(const UI::Theme& theme,
                                               const std::optional<std::string>& variant,
                                               const std::string& key)
    {
      if (variant)
      {
        auto variant_it = theme.vars.find(*variant);
        if (variant_it != theme.vars.end())
        {
          auto var_it = variant_it->second.find(key);
          if (var_it != variant_it->second.end()) return var_it->second;
        }
      }

      if (theme.default_variant)
      {
        auto default_it = theme.vars.find(*theme.default_variant);
        if (default_it != theme.vars.end())
        {
          auto var_it = default_it->second.find(key);
          if (var_it != default_it->second.end()) return var_it->second;
        }
      }

      return nullptr;
    }

    Lisple::sptr_val resolve_theme_vars_internal(const UI::Theme& theme,
                                                 const std::optional<std::string>& variant,
                                                 const Lisple::sptr_val& value,
                                                 int depth = 0)
    {
      if (!value || depth > 32) return value;

      if (is_theme_var_ref(value))
      {
        auto var_value = lookup_theme_var_internal(theme, variant, theme_var_ref_name(value));
        if (!var_value) return nullptr;
        return resolve_theme_vars_internal(theme, variant, var_value, depth + 1);
      }

      switch (value->type)
      {
      case Lisple::Value::Type::LIST:
      {
        Lisple::sptr_val_v elements;
        elements.reserve(value->elements().size());
        for (const auto& child : value->elements())
        {
          auto resolved_child = resolve_theme_vars_internal(theme, variant, child, depth + 1);
          if (resolved_child) elements.push_back(resolved_child);
        }
        return Lisple::list(elements);
      }
      case Lisple::Value::Type::VECTOR:
      {
        Lisple::sptr_val_v elements;
        elements.reserve(value->elements().size());
        for (const auto& child : value->elements())
        {
          auto resolved_child = resolve_theme_vars_internal(theme, variant, child, depth + 1);
          if (resolved_child) elements.push_back(resolved_child);
        }
        return Lisple::vector(elements);
      }
      case Lisple::Value::Type::MAP:
      {
        Lisple::sptr_val_v elements;
        elements.reserve(value->elements().size());
        const auto& source = value->elements();
        for (size_t i = 0; i + 1 < source.size(); i += 2)
        {
          auto resolved_child =
            resolve_theme_vars_internal(theme, variant, source[i + 1], depth + 1);
          if (!resolved_child) continue;
          elements.push_back(source[i]);
          elements.push_back(resolved_child);
        }
        return Lisple::map(elements);
      }
      default:
        return value;
      }
    }

    UI::Style coerce_theme_style(Lisple::Context& ctx, const Lisple::sptr_val& style_val)
    {
      auto mutable_style_val = style_val;
      auto style_coercion = HostType::STYLE.coerce(ctx, mutable_style_val);
      if (!style_coercion.success)
      {
        throw Lisple::TypeError("Invalid theme style declaration: " +
                                style_val->to_string());
      }
      return Lisple::obj<UI::Style>(*style_coercion.result);
    }

    std::set<std::string> variant_names_for_theme(const UI::Theme& theme)
    {
      std::set<std::string> names;
      for (const auto& [variant, _] : theme.vars)
      {
        names.insert(variant);
      }
      if (theme.default_variant) names.insert(*theme.default_variant);
      return names;
    }

    void apply_selector_pseudo_suffixes(UI::ThemeSelector& selector)
    {
      while (true)
      {
        size_t split_idx = selector.value.rfind(':');
        if (split_idx == std::string::npos || split_idx == 0)
        {
          return;
        }

        std::string suffix = selector.value.substr(split_idx + 1);
        if (suffix == "hover")
        {
          selector.hovered = true;
        }
        else if (suffix == "focus")
        {
          selector.focused = true;
        }
        else if (suffix == "focus-within")
        {
          selector.focus_within = true;
        }
        else
        {
          return;
        }

        selector.value = selector.value.substr(0, split_idx);
      }
    }

    std::vector<std::string> parse_theme_extends(const Lisple::sptr_val& value)
    {
      std::vector<std::string> names;
      if (!value || value->type == Lisple::Value::Type::NIL) return names;

      if (value->type == Lisple::Value::Type::SYMBOL)
      {
        names.push_back(value->str());
        return names;
      }

      if (value->type == Lisple::Value::Type::VECTOR)
      {
        for (auto& child : Lisple::get_children(*value))
        {
          if (child->type != Lisple::Value::Type::SYMBOL)
            throw Lisple::TypeError("Theme :extend entries must be symbols");
          names.push_back(child->str());
        }
        return names;
      }

      throw Lisple::TypeError("Theme :extend must be a symbol or vector of symbols");
    }

    Lisple::sptr_val normalize_selector_literal_value(const Lisple::sptr_val& value)
    {
      if (!value) return value;

      switch (value->type)
      {
      case Lisple::Value::Type::SYMBOL:
        if (value->str() == "true") return Lisple::Constant::BOOL_TRUE;
        if (value->str() == "false") return Lisple::Constant::BOOL_FALSE;
        if (value->str() == "nil") return Lisple::Constant::NIL;
        return value;
      case Lisple::Value::Type::LIST:
      {
        Lisple::sptr_val_v elements;
        elements.reserve(value->elements().size());
        for (const auto& child : value->elements())
        {
          elements.push_back(normalize_selector_literal_value(child));
        }
        return Lisple::list(elements);
      }
      case Lisple::Value::Type::VECTOR:
      {
        Lisple::sptr_val_v elements;
        elements.reserve(value->elements().size());
        for (const auto& child : value->elements())
        {
          elements.push_back(normalize_selector_literal_value(child));
        }
        return Lisple::vector(elements);
      }
      case Lisple::Value::Type::MAP:
      {
        Lisple::sptr_val_v elements;
        elements.reserve(value->elements().size());
        const auto& source = value->elements();
        for (size_t i = 0; i + 1 < source.size(); i += 2)
        {
          elements.push_back(source[i]);
          elements.push_back(normalize_selector_literal_value(source[i + 1]));
        }
        return Lisple::map(elements);
      }
      default:
        return value;
      }
    }

    UI::ThemeSelector parse_theme_selector(const Lisple::sptr_val& key)
    {
      switch (key->type)
      {
      case Lisple::Value::Type::SYMBOL:
      {
        auto selector = UI::ThemeSelector::component_type(key->str());
        apply_selector_pseudo_suffixes(selector);
        return selector;
      }
      case Lisple::Value::Type::KEYWORD:
      {
        auto selector = UI::ThemeSelector::class_name(key->str());
        apply_selector_pseudo_suffixes(selector);
        return selector;
      }
      case Lisple::Value::Type::MAP:
        return UI::ThemeSelector::state_match(normalize_selector_literal_value(key));
      case Lisple::Value::Type::LIST:
      {
        std::vector<UI::ThemeSelector> children;
        for (const auto& child : Lisple::get_children(*key))
        {
          auto selector = parse_theme_selector(child);
          if (selector.type == UI::ThemeSelector::Type::DESCENDANT)
            throw Lisple::TypeError(
              "Theme compound selectors cannot contain descendant selectors");
          children.push_back(std::move(selector));
        }
        if (children.empty())
          throw Lisple::TypeError("Theme compound selectors cannot be empty");
        if (children.size() == 1) return children[0];
        return UI::ThemeSelector::compound(children);
      }
      case Lisple::Value::Type::VECTOR:
      {
        std::vector<UI::ThemeSelector> children;
        for (const auto& child : Lisple::get_children(*key))
        {
          auto selector = parse_theme_selector(child);
          if (selector.type == UI::ThemeSelector::Type::DESCENDANT)
            throw Lisple::TypeError(
              "Theme descendant selectors cannot contain descendant selectors");
          children.push_back(std::move(selector));
        }
        if (children.empty())
          throw Lisple::TypeError("Theme descendant selectors cannot be empty");
        if (children.size() == 1) return children[0];
        return UI::ThemeSelector::descendant(children);
      }
      default:
        throw Lisple::TypeError(
          "Theme style selectors must be symbols, keywords, maps, lists, or vectors");
      }
    }
  } // namespace

  Lisple::sptr_val lookup_theme_var(const UI::Theme& theme,
                                    const std::optional<std::string>& variant,
                                    const std::string& key)
  {
    return lookup_theme_var_internal(theme, variant, key);
  }

  Lisple::sptr_val resolve_theme_vars(const UI::Theme& theme,
                                      const std::optional<std::string>& variant,
                                      const Lisple::sptr_val& value,
                                      int depth)
  {
    return resolve_theme_vars_internal(theme, variant, value, depth);
  }

  UI::Theme build_theme_from_definition(Lisple::Context& ctx,
                                        const std::string& name,
                                        const Lisple::sptr_val& definition_map,
                                        const UI::Theme* base)
  {
    UI::Theme theme;

    if (base) theme = *base;
    theme.name = name;

    auto extend_val = Lisple::Dict::get_property(definition_map, Lisple::keyword("extend"));
    theme.extend = parse_theme_extends(extend_val);

    if (!theme.extend.empty())
    {
      auto themes = ctx.lookup(ID__PIXILS__THEMES);
      UI::Theme merged;

      for (const auto& extends_name : theme.extend)
      {
        auto base_val = Lisple::Dict::get_property(themes, Lisple::symbol(extends_name));
        if (!base_val || base_val->type == Lisple::Value::Type::NIL)
          throw Lisple::InvocationException("deftheme :extend - unknown base theme '" +
                                            extends_name + "'");
        UI::overlay_theme(merged, Lisple::obj<UI::Theme>(*base_val));
      }

      if (base) UI::overlay_theme(merged, *base);
      merged.name = name;
      merged.extend = theme.extend;
      theme = merged;
    }

    auto default_variant_val =
      Lisple::Dict::get_property(definition_map, Lisple::keyword("default-variant"));
    auto default_variant = parse_default_variant(default_variant_val);
    if (!default_variant.empty()) theme.default_variant = default_variant;

    auto vars_val = Lisple::Dict::get_property(definition_map, Lisple::keyword("vars"));
    auto local_vars = parse_theme_vars(vars_val);
    for (const auto& [variant, vars] : local_vars)
    {
      auto& out_vars = theme.vars[variant];
      for (const auto& [key, value] : vars)
      {
        out_vars[key] = value;
      }
    }

    if (!theme.vars.empty() && !theme.default_variant)
    {
      throw Lisple::TypeError("Theme :vars requires :default-variant");
    }

    auto styles_val = Lisple::Dict::get_property(definition_map, Lisple::keyword("styles"));
    if (styles_val && styles_val->type == Lisple::Value::Type::MAP)
    {
      auto variant_names = variant_names_for_theme(theme);
      for (auto& key : Lisple::Dict::keys(*styles_val))
      {
        auto style_val = Lisple::Dict::get_property(styles_val, key);
        auto selector = parse_theme_selector(key);

        if (variant_names.empty())
        {
          auto resolved_value = resolve_theme_vars(theme, std::nullopt, style_val);
          if (resolved_value)
          {
            theme.set_style(selector, coerce_theme_style(ctx, resolved_value));
          }
          continue;
        }

        for (const auto& variant : variant_names)
        {
          auto resolved_value = resolve_theme_vars(theme, variant, style_val);
          if (!resolved_value) continue;
          auto resolved_style = coerce_theme_style(ctx, resolved_value);
          theme.set_variant_style(variant, selector, resolved_style);
          if (theme.default_variant && variant == *theme.default_variant)
          {
            theme.set_style(selector, resolved_style);
          }
        }
      }
    }

    return theme;
  }
} // namespace Pixils::Script
