#include "pixils/binding/theme_definition.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/style_namespace.h>
#include <pixils/ui/theme.h>

#include <lisple/context.h>
#include <lisple/exception.h>
#include <lisple/host.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>

namespace Pixils::Script
{
  namespace
  {
    std::vector<std::string> parse_theme_extends(const Lisple::sptr_rtval& value)
    {
      std::vector<std::string> names;
      if (!value || value->type == Lisple::RTValue::Type::NIL) return names;

      if (value->type == Lisple::RTValue::Type::SYMBOL)
      {
        names.push_back(value->str());
        return names;
      }

      if (value->type == Lisple::RTValue::Type::VECTOR)
      {
        for (auto& child : Lisple::get_children(*value))
        {
          if (child->type != Lisple::RTValue::Type::SYMBOL)
            throw Lisple::TypeError("Theme :extend entries must be symbols");
          names.push_back(child->str());
        }
        return names;
      }

      throw Lisple::TypeError("Theme :extend must be a symbol or vector of symbols");
    }

    Lisple::sptr_rtval normalize_selector_literal_value(const Lisple::sptr_rtval& value)
    {
      if (!value) return value;

      switch (value->type)
      {
      case Lisple::RTValue::Type::SYMBOL:
        if (value->str() == "true") return Lisple::Constant::BOOL_TRUE;
        if (value->str() == "false") return Lisple::Constant::BOOL_FALSE;
        if (value->str() == "nil") return Lisple::Constant::NIL;
        return value;
      case Lisple::RTValue::Type::LIST:
      {
        Lisple::sptr_rtval_v elements;
        elements.reserve(value->elements().size());
        for (const auto& child : value->elements())
        {
          elements.push_back(normalize_selector_literal_value(child));
        }
        return Lisple::RTValue::list(elements);
      }
      case Lisple::RTValue::Type::VECTOR:
      {
        Lisple::sptr_rtval_v elements;
        elements.reserve(value->elements().size());
        for (const auto& child : value->elements())
        {
          elements.push_back(normalize_selector_literal_value(child));
        }
        return Lisple::RTValue::vector(elements);
      }
      case Lisple::RTValue::Type::MAP:
      {
        Lisple::sptr_rtval_v elements;
        elements.reserve(value->elements().size());
        const auto& source = value->elements();
        for (size_t i = 0; i + 1 < source.size(); i += 2)
        {
          elements.push_back(source[i]);
          elements.push_back(normalize_selector_literal_value(source[i + 1]));
        }
        return Lisple::RTValue::map(elements);
      }
      default:
        return value;
      }
    }

    UI::ThemeSelector parse_theme_selector(const Lisple::sptr_rtval& key)
    {
      switch (key->type)
      {
      case Lisple::RTValue::Type::SYMBOL:
        return UI::ThemeSelector::component_type(key->str());
      case Lisple::RTValue::Type::KEYWORD:
        return UI::ThemeSelector::class_name(key->str());
      case Lisple::RTValue::Type::MAP:
        return UI::ThemeSelector::state_match(normalize_selector_literal_value(key));
      case Lisple::RTValue::Type::LIST:
      {
        std::vector<UI::ThemeSelector> children;
        for (const auto& child : Lisple::get_children(*key))
        {
          auto selector = parse_theme_selector(child);
          if (selector.type == UI::ThemeSelector::Type::DESCENDANT)
            throw Lisple::TypeError("Theme compound selectors cannot contain descendant selectors");
          children.push_back(std::move(selector));
        }
        if (children.empty())
          throw Lisple::TypeError("Theme compound selectors cannot be empty");
        if (children.size() == 1) return children[0];
        return UI::ThemeSelector::compound(children);
      }
      case Lisple::RTValue::Type::VECTOR:
      {
        std::vector<UI::ThemeSelector> children;
        for (const auto& child : Lisple::get_children(*key))
        {
          auto selector = parse_theme_selector(child);
          if (selector.type == UI::ThemeSelector::Type::DESCENDANT)
            throw Lisple::TypeError("Theme descendant selectors cannot contain descendant selectors");
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

  UI::Theme build_theme_from_definition(Lisple::Context& ctx,
                                        const std::string& name,
                                        const Lisple::sptr_rtval& definition_map,
                                        const UI::Theme* base)
  {
    UI::Theme theme;

    if (base) theme = *base;
    theme.name = name;

    auto extend_val =
      Lisple::Dict::get_property(definition_map, Lisple::RTValue::keyword("extend"));
    theme.extend = parse_theme_extends(extend_val);

    if (!theme.extend.empty())
    {
      auto themes = ctx.lookup_value(ID__PIXILS__THEMES);
      UI::Theme merged;

      for (const auto& extends_name : theme.extend)
      {
        auto base_val =
          Lisple::Dict::get_property(themes, Lisple::RTValue::symbol(extends_name));
        if (!base_val || base_val->type == Lisple::RTValue::Type::NIL)
          throw Lisple::InvocationException("deftheme :extend - unknown base theme '" +
                                            extends_name + "'");
        UI::overlay_theme(merged, Lisple::obj<UI::Theme>(*base_val));
      }

      if (base) UI::overlay_theme(merged, *base);
      merged.name = name;
      merged.extend = theme.extend;
      theme = merged;
    }

    auto styles_val =
      Lisple::Dict::get_property(definition_map, Lisple::RTValue::keyword("styles"));
    if (styles_val && styles_val->type == Lisple::RTValue::Type::MAP)
    {
      for (auto& key : Lisple::Dict::keys(*styles_val))
      {
        auto style_val = Lisple::Dict::get_property(styles_val, key);
        auto style_coercion = HostType::STYLE.coerce(ctx, style_val);
        if (!style_coercion.success)
          throw Lisple::TypeError("Invalid theme style declaration: " +
                                  style_val->to_string());

        theme.set_style(parse_theme_selector(key),
                        Lisple::obj<UI::Style>(*style_coercion.result));
      }
    }

    return theme;
  }
} // namespace Pixils::Script
