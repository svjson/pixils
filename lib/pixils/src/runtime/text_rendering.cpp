#include "pixils/runtime/text_rendering.h"

#include <pixils/binding/color_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/context.h>

#include <roo/context.h>
#include <roo/exception.h>
#include <roo/host/schema.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <string>
#include <vector>

namespace Pixils::Runtime
{
  namespace
  {
    Roo::MapSchema text_options_schema({},
                                       {{"font", &Roo::Type::KEYWORD},
                                        {"color", &Script::HostType::COLOR},
                                        {"scale", &Roo::Type::ANY},
                                        {"font-styles", &Roo::Type::ANY},
                                        {"shadow", &Roo::Type::ANY},
                                        {"marked-style", &Roo::Type::ANY}});

    Text::Scale parse_text_scale(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return Text::Scale(1);
      if (value->type == Roo::Value::Type::NUMBER) return Text::Scale(value->f32());
      if (value->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("Text scale must be a number or [x y] vector");
      }

      auto children = Roo::get_children(*value);
      if (children.size() != 2 || children[0]->type != Roo::Value::Type::NUMBER ||
          children[1]->type != Roo::Value::Type::NUMBER)
      {
        throw Roo::TypeError("Text scale vector must be [x y] numbers");
      }
      return Text::Scale(children[0]->f32(), children[1]->f32());
    }

    std::vector<Text::FontStyle> parse_font_styles(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return {};

      auto parse_one = [](const Roo::sptr_val& style_value)
      {
        if (!style_value || style_value->type != Roo::Value::Type::KEYWORD)
        {
          throw Roo::TypeError("Text font style must be a keyword");
        }

        if (style_value->str() == "bold") return Text::FontStyle::BOLD;
        if (style_value->str() == "underline") return Text::FontStyle::UNDERLINE;
        throw Roo::TypeError("Unknown text font style: " + style_value->to_string());
      };

      if (value->type == Roo::Value::Type::KEYWORD) return {parse_one(value)};
      if (value->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("Text font styles must be a keyword or vector");
      }

      std::vector<Text::FontStyle> result;
      for (auto& child : Roo::get_children(*value))
      {
        result.push_back(parse_one(child));
      }
      return result;
    }

    std::vector<Text::Shadow> parse_shadows(Roo::Context& ctx, const Roo::sptr_val& value)
    {
      std::vector<Text::Shadow> shadows;
      if (!value || value->type == Roo::Value::Type::NIL) return shadows;

      static Roo::MapSchema shadow_schema(
        {{"offset", &Script::HostType::POINT}, {"color", &Script::HostType::COLOR}},
        {});

      auto parse_one = [&](const Roo::sptr_val& shadow)
      {
        auto bound = shadow_schema.bind(ctx, *shadow);
        return Text::Shadow(bound.obj<Point>("offset"), bound.obj<Color>("color"));
      };

      if (value->type == Roo::Value::Type::VECTOR)
      {
        for (auto& shadow : Roo::get_children(*value))
        {
          shadows.push_back(parse_one(shadow));
        }
      }
      else if (value->type == Roo::Value::Type::MAP)
      {
        shadows.push_back(parse_one(value));
      }

      return shadows;
    }

    std::optional<char> parse_inline_marker(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;
      if (value->type == Roo::Value::Type::CHAR) return static_cast<char>(value->ch());
      if (value->type == Roo::Value::Type::STRING ||
          value->type == Roo::Value::Type::KEYWORD ||
          value->type == Roo::Value::Type::SYMBOL)
      {
        std::string marker = value->str();
        if (marker.size() == 1) return marker[0];
      }
      return std::nullopt;
    }

    std::optional<Text::InlineTextStyleSpec> parse_marked_style(Roo::Context& ctx,
                                                                const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

      static Roo::MapSchema marked_style_schema({},
                                                {{"enabled", &Roo::Type::BOOL},
                                                 {"marker", &Roo::Type::ANY},
                                                 {"font", &Roo::Type::KEYWORD},
                                                 {"color", &Script::HostType::COLOR},
                                                 {"scale", &Roo::Type::ANY},
                                                 {"font-styles", &Roo::Type::ANY},
                                                 {"shadow", &Roo::Type::ANY}});

      auto source = value;
      if (Roo::Dict::contains_key(*value, "color"))
      {
        auto color = Roo::Dict::get_property(*value, "color");
        if (color && color->type == Roo::Value::Type::KEYWORD && color->str() == "none")
        {
          source = Roo::Dict::shallow_copy(value);
          Roo::Dict::set_property(source, Roo::keyword("color"), Roo::Constant::NIL);
        }
      }

      auto options = marked_style_schema.bind(ctx, *source);
      Text::InlineTextStyleSpec spec;
      spec.enabled = options.contains("enabled") ? options.boolean("enabled") : true;
      if (auto marker = parse_inline_marker(options.val("marker")); marker.has_value())
      {
        spec.marker = *marker;
      }
      if (options.contains("font")) spec.font_key = options.str("font");
      if (auto color = options.val("color");
          color && color->type == Roo::Value::Type::KEYWORD && color->str() == "none")
      {
        spec.use_font_color = true;
      }
      else if (auto color = options.val("color");
               color && color->type != Roo::Value::Type::NIL)
      {
        spec.color = Roo::obj<Color>(*color);
      }
      if (options.contains("scale")) spec.scale = parse_text_scale(options.val("scale"));
      if (options.contains("font-styles"))
      {
        spec.font_styles = parse_font_styles(options.val("font-styles"));
      }
      if (options.contains("shadow"))
      {
        spec.shadows = parse_shadows(ctx, options.val("shadow"));
      }
      return spec;
    }
  } // namespace

  std::optional<Text::TextRenderOp> resolve_text_render_op(
    Roo::Context& ctx,
    RenderContext& render_context,
    const Roo::sptr_val& options_value)
  {
    auto options = text_options_schema.bind(ctx, *options_value);
    std::string font_key = "font/console";
    if (auto value = options.val("font"); value && value->type == Roo::Value::Type::KEYWORD)
    {
      font_key = value->str();
    }

    Text::Scale scale(1);
    if (auto value = options.val("scale"); value && value->type != Roo::Value::Type::NIL)
    {
      scale = parse_text_scale(value);
    }

    std::optional<Color> color;
    if (auto value = options.val("color"); value && value->type != Roo::Value::Type::NIL)
    {
      color = Roo::obj<Color>(*value);
    }

    std::vector<Text::FontStyle> font_styles;
    if (auto value = options.val("font-styles");
        value && value->type != Roo::Value::Type::NIL)
    {
      font_styles = parse_font_styles(value);
    }

    std::vector<Text::Shadow> shadows;
    if (auto value = options.val("shadow"); value && value->type != Roo::Value::Type::NIL)
    {
      shadows = parse_shadows(ctx, value);
    }

    return Text::make_text_render_op(render_context,
                                     font_key,
                                     scale,
                                     color,
                                     font_styles,
                                     shadows,
                                     parse_marked_style(ctx, options.val("marked-style")));
  }
} // namespace Pixils::Runtime
