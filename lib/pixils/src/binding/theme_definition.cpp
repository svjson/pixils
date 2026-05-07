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

    UI::ThemeSelector parse_theme_selector(const Lisple::sptr_rtval& key)
    {
      switch (key->type)
      {
      case Lisple::RTValue::Type::SYMBOL:
        return UI::ThemeSelector::component_type(key->str());
      case Lisple::RTValue::Type::KEYWORD:
        return UI::ThemeSelector::class_name(key->str());
      default:
        throw Lisple::TypeError("Theme style selectors must be symbols or keywords");
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
