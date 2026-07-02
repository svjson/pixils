#include "pixils/binding/ui/style/style_definition.h"

#include <pixils/binding/color_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/ui/style/style_host_type.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/host/object.h>
#include <roo/host/schema.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/type.h>
#include <vector>

namespace Pixils::Script::StyleDefinition
{
  namespace
  {
    Roo::sptr_val map_like_value(const Roo::sptr_val& value)
    {
      if (!value) return nullptr;
      if (value->type == Roo::Value::Type::MAP) return value;
      if (value->type == Roo::Value::Type::NATIVE_OBJECT &&
          value->nobj()->structural_kind() == Roo::NativeObjectStructuralKind::MAP)
      {
        return Roo::map(value->nobj()->native_children());
      }
      return nullptr;
    }

    std::optional<uint8_t> parse_color_channel(const Roo::sptr_val& value,
                                               const std::string& key)
    {
      auto channel = Roo::Dict::get_property(value, Roo::keyword(key));
      if (!channel || channel->type == Roo::Value::Type::NIL) return std::nullopt;
      if (channel->type != Roo::Value::Type::NUMBER) return std::nullopt;
      return channel->ui8();
    }

    std::optional<Color> parse_color_value(Roo::Context&, const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

      if (HostType::COLOR.is_type_of(*value))
      {
        return Roo::obj<Color>(*value);
      }

      if (value->type != Roo::Value::Type::MAP &&
          value->type != Roo::Value::Type::NATIVE_OBJECT)
      {
        return std::nullopt;
      }

      auto r = parse_color_channel(value, "r");
      auto g = parse_color_channel(value, "g");
      auto b = parse_color_channel(value, "b");
      if (!r || !g || !b) return std::nullopt;

      auto a = parse_color_channel(value, "a").value_or(0xff);
      return Color{*r, *g, *b, a};
    }

    std::optional<Color> optional_color(Roo::Context& ctx,
                                        Roo::MapSchema::Inspector& opts,
                                        const std::string& key)
    {
      if (!opts.contains(key)) return std::nullopt;
      if (opts.val(key)->type == Roo::Value::Type::NIL) return std::nullopt;

      auto color = parse_color_value(ctx, opts.val(key));
      if (!color)
      {
        throw Roo::TypeError("Invalid color declaration: " + opts.val(key)->to_string());
      }
      return color;
    }

    Color required_color(Roo::Context& ctx, const Roo::sptr_val& value)
    {
      auto color = parse_color_value(ctx, value);
      if (!color)
      {
        throw Roo::TypeError("Invalid color declaration: " + value->to_string());
      }
      return *color;
    }

    void apply_border_props(Roo::Context& ctx,
                            UI::Style::Border& border,
                            Roo::MapSchema::Inspector& opts)
    {
      if (opts.contains("thickness")) border.thickness = opts.i32("thickness");
      border.line_style = parse_line_style(opts.val("line-style"));
      border.color = optional_color(ctx, opts, "color");
      border.trim = parse_trim(opts.val("trim"));
    }

    std::optional<Pixils::Text::FontStyle> parse_font_style_keyword(
      const Roo::sptr_val& value)
    {
      if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
      if (value->str() == "underline") return Pixils::Text::FontStyle::UNDERLINE;
      return std::nullopt;
    }

    std::optional<std::vector<Pixils::Text::FontStyle>> parse_text_font_styles(
      const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

      if (auto single = parse_font_style_keyword(value))
      {
        return std::vector<Pixils::Text::FontStyle>{*single};
      }

      if (value->type != Roo::Value::Type::VECTOR) return std::nullopt;

      std::vector<Pixils::Text::FontStyle> styles;
      for (auto& child : Roo::get_children(*value))
      {
        auto parsed = parse_font_style_keyword(child);
        if (!parsed) return std::nullopt;
        styles.push_back(*parsed);
      }
      return styles;
    }

    std::optional<std::vector<Pixils::Text::Shadow>> parse_text_shadows(
      Roo::Context& ctx,
      const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

      static Roo::MapSchema shadow_schema({{"offset", &HostType::POINT},
                                              {"color", &Roo::Type::ANY}},
                                             {});

      auto parse_one = [&](const Roo::sptr_val& shadow_value)
      {
        auto sh = shadow_schema.bind(ctx, *shadow_value);
        return Pixils::Text::Shadow(sh.obj<Point>("offset"),
                                    required_color(ctx, sh.val("color")));
      };

      std::vector<Pixils::Text::Shadow> shadows;
      if (value->type == Roo::Value::Type::VECTOR)
      {
        for (auto& child : Roo::get_children(*value))
        {
          shadows.push_back(parse_one(child));
        }
        return shadows;
      }

      if (value->type == Roo::Value::Type::MAP)
      {
        shadows.push_back(parse_one(value));
        return shadows;
      }

      return std::nullopt;
    }

    std::optional<UI::Style::Background::Fit> parse_background_fit(
      const Roo::sptr_val& value)
    {
      if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
      if (value->str() == "none") return UI::Style::Background::Fit::NONE;
      if (value->str() == "contain") return UI::Style::Background::Fit::CONTAIN;
      if (value->str() == "cover") return UI::Style::Background::Fit::COVER;
      if (value->str() == "fill") return UI::Style::Background::Fit::FILL;
      return std::nullopt;
    }

    std::optional<UI::Style::Background::Align> parse_background_align_keyword(
      const Roo::sptr_val& value)
    {
      if (!value || value->type != Roo::Value::Type::KEYWORD)
      {
        return std::nullopt;
      }
      if (value->str() == "start" || value->str() == "left" || value->str() == "top")
      {
        return UI::Style::Background::Align::START;
      }
      if (value->str() == "center")
      {
        return UI::Style::Background::Align::CENTER;
      }
      if (value->str() == "end" || value->str() == "right" || value->str() == "bottom")
      {
        return UI::Style::Background::Align::END;
      }
      return std::nullopt;
    }

    void apply_background_align(UI::Style::Background& bg, const Roo::sptr_val& value)
    {
      if (!value || value->type != Roo::Value::Type::KEYWORD) return;

      auto name = value->str();
      if (name == "top-left")
      {
        bg.align_x = UI::Style::Background::Align::START;
        bg.align_y = UI::Style::Background::Align::START;
      }
      else if (name == "top")
      {
        bg.align_x = UI::Style::Background::Align::CENTER;
        bg.align_y = UI::Style::Background::Align::START;
      }
      else if (name == "top-right")
      {
        bg.align_x = UI::Style::Background::Align::END;
        bg.align_y = UI::Style::Background::Align::START;
      }
      else if (name == "left")
      {
        bg.align_x = UI::Style::Background::Align::START;
        bg.align_y = UI::Style::Background::Align::CENTER;
      }
      else if (name == "center")
      {
        bg.align_x = UI::Style::Background::Align::CENTER;
        bg.align_y = UI::Style::Background::Align::CENTER;
      }
      else if (name == "right")
      {
        bg.align_x = UI::Style::Background::Align::END;
        bg.align_y = UI::Style::Background::Align::CENTER;
      }
      else if (name == "bottom-left")
      {
        bg.align_x = UI::Style::Background::Align::START;
        bg.align_y = UI::Style::Background::Align::END;
      }
      else if (name == "bottom")
      {
        bg.align_x = UI::Style::Background::Align::CENTER;
        bg.align_y = UI::Style::Background::Align::END;
      }
      else if (name == "bottom-right")
      {
        bg.align_x = UI::Style::Background::Align::END;
        bg.align_y = UI::Style::Background::Align::END;
      }
      else if (auto align = parse_background_align_keyword(value))
      {
        bg.align_x = align;
        bg.align_y = align;
      }
    }

    std::optional<char> parse_inline_marker(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

      if (value->type == Roo::Value::Type::CHAR)
      {
        return static_cast<char>(value->ch());
      }

      if (value->type == Roo::Value::Type::STRING ||
          value->type == Roo::Value::Type::KEYWORD ||
          value->type == Roo::Value::Type::SYMBOL)
      {
        std::string raw = value->str();
        if (raw.size() == 1) return raw[0];
      }

      return std::nullopt;
    }

    std::optional<UI::Style::Text::MarkedStyle> parse_marked_style(
      Roo::Context& ctx,
      const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;
      auto source = map_like_value(value);
      if (!source) return std::nullopt;

      static Roo::MapSchema inline_schema({},
                                             {{"enabled", &Roo::Type::BOOL},
                                              {"marker", &Roo::Type::ANY},
                                              {"color", &Roo::Type::ANY},
                                              {"font", &Roo::Type::KEYWORD},
                                              {"scale", &Roo::Type::NUMBER},
                                              {"font-styles", &Roo::Type::ANY},
                                              {"shadow", &Roo::Type::ANY}});

      auto inline_source = source;
      if (Roo::Dict::contains_key(*source, "color"))
      {
        auto color_value = Roo::Dict::get_property(*source, "color");
        if (parse_text_use_font_color(color_value))
        {
          inline_source = Roo::Dict::shallow_copy(source);
          Roo::Dict::set_property(inline_source,
                                     Roo::keyword("color"),
                                     Roo::Constant::NIL);
        }
      }

      auto opts = inline_schema.bind(ctx, *inline_source);
      UI::Style::Text::MarkedStyle marked_style;
      if (opts.contains("enabled")) marked_style.enabled = opts.boolean("enabled");

      if (auto marker = parse_inline_marker(opts.val("marker")); marker.has_value())
      {
        marked_style.marker = *marker;
      }

      auto color_value = opts.val("color");
      if (parse_text_use_font_color(color_value))
      {
        marked_style.use_font_color = true;
      }
      else if (opts.contains("color"))
      {
        marked_style.use_font_color = false;
        marked_style.color = optional_color(ctx, opts, "color");
      }

      if (opts.contains("font")) marked_style.font = opts.str("font");
      if (opts.contains("scale")) marked_style.scale = parse_text_scale(opts.val("scale"));
      if (opts.contains("font-styles"))
      {
        marked_style.font_styles = parse_text_font_styles(opts.val("font-styles"));
      }
      if (opts.contains("shadow"))
      {
        marked_style.shadows = parse_text_shadows(ctx, opts.val("shadow"));
      }

      return marked_style;
    }
  } // namespace

  std::optional<UI::Style::Size> parse_size(const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

    switch (value->type)
    {
    case Roo::Value::Type::NUMBER:
      return UI::Style::Size(value->num().get_int());
    case Roo::Value::Type::KEYWORD:
      if (value->str() == "fill") return UI::Style::Size(UI::Style::Size::Mode::FILL);
      if (value->str() == "shrink") return UI::Style::Size(UI::Style::Size::Mode::SHRINK);
      if (value->str() == "auto") return UI::Style::Size(UI::Style::Size::Mode::AUTO);
      return std::nullopt;
    default:
      return std::nullopt;
    }
  }

  Roo::sptr_val size_to_value(const std::optional<UI::Style::Size>& size)
  {
    if (!size) return Roo::Constant::NIL;
    if (size->is_fixed()) return Roo::number(size->fixed_value_or(0));
    if (size->is_fill()) return Roo::keyword("fill");
    if (size->is_shrink()) return Roo::keyword("shrink");
    return Roo::keyword("auto");
  }

  std::optional<UI::Style::Trim> parse_trim(const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

    switch (value->type)
    {
    case Roo::Value::Type::NUMBER:
      return UI::Style::Trim{value->num().get_int()};
    case Roo::Value::Type::VECTOR:
      switch (Roo::count(*value))
      {
      case 1:
        return UI::Style::Trim{Roo::get_child(*value, 0)->num().get_int()};
      case 2:
        return UI::Style::Trim{Roo::get_child(*value, 0)->num().get_int(),
                               Roo::get_child(*value, 1)->num().get_int()};
      default:
        return std::nullopt;
      }
    default:
      return std::nullopt;
    }
  }

  Roo::sptr_val trim_to_value(const std::optional<UI::Style::Trim>& trim)
  {
    if (!trim) return Roo::Constant::NIL;
    return Roo::vector({Roo::number(trim->start), Roo::number(trim->end)});
  }

  std::optional<UI::Style::CornerRadius> parse_corner_radius(Roo::Context& ctx,
                                                             const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

    if (value->type == Roo::Value::Type::NUMBER)
    {
      return UI::Style::CornerRadius(std::max(0, value->num().get_int()));
    }

    auto source = map_like_value(value);
    if (!source) return std::nullopt;

    static Roo::MapSchema corner_radius_schema({},
                                                  {{"tl", &Roo::Type::NUMBER},
                                                   {"tr", &Roo::Type::NUMBER},
                                                   {"br", &Roo::Type::NUMBER},
                                                   {"bl", &Roo::Type::NUMBER}});

    auto opts = corner_radius_schema.bind(ctx, *source);
    return UI::Style::CornerRadius(std::max(0, opts.i32("tl", 0)),
                                   std::max(0, opts.i32("tr", 0)),
                                   std::max(0, opts.i32("br", 0)),
                                   std::max(0, opts.i32("bl", 0)));
  }

  Roo::sptr_val corner_radius_to_value(
    const std::optional<UI::Style::CornerRadius>& radius)
  {
    if (!radius) return Roo::Constant::NIL;
    if (radius->tl == radius->tr && radius->tl == radius->br && radius->tl == radius->bl)
    {
      return Roo::number(radius->tl);
    }

    return Roo::map({Roo::keyword("tl"),
                        Roo::number(radius->tl),
                        Roo::keyword("tr"),
                        Roo::number(radius->tr),
                        Roo::keyword("br"),
                        Roo::number(radius->br),
                        Roo::keyword("bl"),
                        Roo::number(radius->bl)});
  }

  std::optional<UI::PositionMode> parse_position_mode(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    return value->str() == "absolute" ? UI::PositionMode::ABSOLUTE : UI::PositionMode::FLOW;
  }

  Roo::sptr_val position_mode_to_value(const std::optional<UI::PositionMode>& mode)
  {
    if (!mode) return Roo::Constant::NIL;
    return Roo::keyword(*mode == UI::PositionMode::ABSOLUTE ? "absolute" : "flow");
  }

  std::optional<float> parse_opacity(Roo::MapSchema::Inspector& opts,
                                     const std::string& key)
  {
    if (!opts.contains(key)) return std::nullopt;
    return std::clamp(opts.f32(key), 0.0f, 1.0f);
  }

  std::optional<UI::Style::BoxSizing> parse_box_sizing(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    return value->str() == "content-box" ? UI::Style::BoxSizing::CONTENT_BOX
                                         : UI::Style::BoxSizing::BORDER_BOX;
  }

  Roo::sptr_val box_sizing_to_value(const std::optional<UI::Style::BoxSizing>& value)
  {
    if (!value) return Roo::Constant::NIL;
    return Roo::keyword(*value == UI::Style::BoxSizing::CONTENT_BOX ? "content-box"
                                                                       : "border-box");
  }

  std::optional<UI::Style::Visibility> parse_visibility(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "visible") return UI::Style::Visibility::VISIBLE;
    if (value->str() == "hidden") return UI::Style::Visibility::HIDDEN;
    if (value->str() == "none") return UI::Style::Visibility::NONE;
    return std::nullopt;
  }

  Roo::sptr_val visibility_to_value(const std::optional<UI::Style::Visibility>& value)
  {
    if (!value) return Roo::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Visibility::VISIBLE:
      return Roo::keyword("visible");
    case UI::Style::Visibility::HIDDEN:
      return Roo::keyword("hidden");
    case UI::Style::Visibility::NONE:
      return Roo::keyword("none");
    }
    return Roo::Constant::NIL;
  }

  std::optional<int> parse_scale(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::NUMBER) return std::nullopt;
    return std::max(1, value->num().get_int());
  }

  Roo::sptr_val scale_to_value(const std::optional<int>& value)
  {
    return value ? Roo::number(*value) : Roo::Constant::NIL;
  }

  std::optional<UI::SystemCursor> parse_system_cursor(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "default") return UI::SystemCursor::DEFAULT;
    if (value->str() == "pointer" || value->str() == "hand")
      return UI::SystemCursor::POINTER;
    if (value->str() == "text") return UI::SystemCursor::TEXT;
    if (value->str() == "crosshair") return UI::SystemCursor::CROSSHAIR;
    if (value->str() == "move") return UI::SystemCursor::MOVE;
    if (value->str() == "not-allowed") return UI::SystemCursor::NOT_ALLOWED;
    if (value->str() == "wait") return UI::SystemCursor::WAIT;
    if (value->str() == "progress") return UI::SystemCursor::PROGRESS;
    if (value->str() == "resize-x" || value->str() == "ew-resize")
      return UI::SystemCursor::RESIZE_X;
    if (value->str() == "resize-y" || value->str() == "ns-resize")
      return UI::SystemCursor::RESIZE_Y;
    if (value->str() == "resize-nwse" || value->str() == "nwse-resize")
      return UI::SystemCursor::RESIZE_NWSE;
    if (value->str() == "resize-nesw" || value->str() == "nesw-resize")
      return UI::SystemCursor::RESIZE_NESW;
    return std::nullopt;
  }

  std::optional<UI::ImageCursor> parse_image_cursor(Roo::Context& ctx,
                                                    const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::MAP) return std::nullopt;

    static Roo::MapSchema pointer_schema({{"image", &Roo::Type::KEYWORD}},
                                            {{"source", &HostType::RECT},
                                             {"hotspot", &HostType::POINT},
                                             {"scale", &Roo::Type::NUMBER},
                                             {"render", &Roo::Type::KEYWORD}});

    auto opts = pointer_schema.bind(ctx, *value);
    UI::ImageCursor pointer;
    pointer.image = opts.val("image")->qual();
    pointer.source = opts.optional_obj<Rect>("source");
    if (opts.contains("hotspot")) pointer.hotspot = opts.obj<Point>("hotspot");
    if (opts.contains("scale")) pointer.scale = std::max(1, opts.i32("scale"));
    if (opts.contains("render") && opts.str("render") == "native")
    {
      pointer.render_mode = UI::ImageCursor::RenderMode::NATIVE;
    }
    return pointer;
  }

  std::optional<UI::CursorSpec> parse_cursor(Roo::Context& ctx,
                                             const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;

    if (auto system = parse_system_cursor(value))
    {
      return UI::CursorSpec::system_cursor(*system);
    }

    if (value->type == Roo::Value::Type::KEYWORD)
    {
      return UI::CursorSpec::named(value->str());
    }

    if (auto image_cursor = parse_image_cursor(ctx, value))
    {
      return UI::CursorSpec::image_cursor(*image_cursor);
    }

    return std::nullopt;
  }

  Roo::sptr_val system_cursor_to_value(const UI::SystemCursor& value)
  {
    switch (value)
    {
    case UI::SystemCursor::DEFAULT:
      return Roo::keyword("default");
    case UI::SystemCursor::POINTER:
      return Roo::keyword("pointer");
    case UI::SystemCursor::TEXT:
      return Roo::keyword("text");
    case UI::SystemCursor::CROSSHAIR:
      return Roo::keyword("crosshair");
    case UI::SystemCursor::MOVE:
      return Roo::keyword("move");
    case UI::SystemCursor::NOT_ALLOWED:
      return Roo::keyword("not-allowed");
    case UI::SystemCursor::WAIT:
      return Roo::keyword("wait");
    case UI::SystemCursor::PROGRESS:
      return Roo::keyword("progress");
    case UI::SystemCursor::RESIZE_X:
      return Roo::keyword("resize-x");
    case UI::SystemCursor::RESIZE_Y:
      return Roo::keyword("resize-y");
    case UI::SystemCursor::RESIZE_NWSE:
      return Roo::keyword("resize-nwse");
    case UI::SystemCursor::RESIZE_NESW:
      return Roo::keyword("resize-nesw");
    }
    return Roo::Constant::NIL;
  }

  Roo::sptr_val cursor_to_value(const std::optional<UI::CursorSpec>& value)
  {
    if (!value) return Roo::Constant::NIL;

    switch (value->kind)
    {
    case UI::CursorSpec::Kind::SYSTEM:
      return system_cursor_to_value(value->system);
    case UI::CursorSpec::Kind::NAMED:
      return Roo::keyword(value->name);
    case UI::CursorSpec::Kind::IMAGE:
    {
      std::vector<Roo::sptr_val> values;
      if (value->image.image)
      {
        values.push_back(Roo::keyword("image"));
        values.push_back(
          Roo::keyword(value->image.image->first + "/" + value->image.image->second));
      }
      if (value->image.source)
      {
        values.push_back(Roo::keyword("source"));
        values.push_back(Roo::map({Roo::keyword("x"),
                                      Roo::number(value->image.source->x),
                                      Roo::keyword("y"),
                                      Roo::number(value->image.source->y),
                                      Roo::keyword("w"),
                                      Roo::number(value->image.source->w),
                                      Roo::keyword("h"),
                                      Roo::number(value->image.source->h)}));
      }
      values.push_back(Roo::keyword("hotspot"));
      values.push_back(Roo::map({Roo::keyword("x"),
                                    Roo::number(value->image.hotspot.round_x()),
                                    Roo::keyword("y"),
                                    Roo::number(value->image.hotspot.round_y())}));
      values.push_back(Roo::keyword("scale"));
      values.push_back(Roo::number(value->image.scale));
      values.push_back(Roo::keyword("render"));
      values.push_back(Roo::keyword(
        value->image.render_mode == UI::ImageCursor::RenderMode::NATIVE ? "native" : "app"));
      return Roo::map(values);
    }
    }

    return Roo::Constant::NIL;
  }

  std::optional<UI::Style::LineStyle> parse_line_style(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "solid") return UI::Style::LineStyle::SOLID;
    if (value->str() == "bevel") return UI::Style::LineStyle::BEVEL;
    return std::nullopt;
  }

  Roo::sptr_val line_style_to_value(const std::optional<UI::Style::LineStyle>& value)
  {
    if (!value) return Roo::Constant::NIL;
    switch (*value)
    {
    case UI::Style::LineStyle::SOLID:
      return Roo::keyword("solid");
    case UI::Style::LineStyle::BEVEL:
      return Roo::keyword("bevel");
    }
    return Roo::Constant::NIL;
  }

  std::optional<Pixils::Text::Alignment> parse_text_align(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "left") return Pixils::Text::Alignment::LEFT;
    if (value->str() == "center") return Pixils::Text::Alignment::CENTER;
    if (value->str() == "right") return Pixils::Text::Alignment::RIGHT;
    return std::nullopt;
  }

  Roo::sptr_val text_align_to_value(const std::optional<Pixils::Text::Alignment>& value)
  {
    if (!value) return Roo::Constant::NIL;
    switch (*value)
    {
    case Pixils::Text::Alignment::LEFT:
      return Roo::keyword("left");
    case Pixils::Text::Alignment::CENTER:
      return Roo::keyword("center");
    case Pixils::Text::Alignment::RIGHT:
      return Roo::keyword("right");
    }
    return Roo::Constant::NIL;
  }

  std::optional<float> parse_text_scale(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::NUMBER) return std::nullopt;
    return std::max(0.01f, value->f32());
  }

  Roo::sptr_val text_scale_to_value(const std::optional<float>& value)
  {
    return value ? Roo::number(*value) : Roo::Constant::NIL;
  }

  bool parse_text_use_font_color(const Roo::sptr_val& value)
  {
    return value && value->type == Roo::Value::Type::KEYWORD && value->str() == "none";
  }

  Roo::sptr_val text_color_to_value(const UI::Style::Text& text)
  {
    if (text.use_font_color) return Roo::keyword("none");
    if (text.color) return ColorAdapter::make_ref(*text.color);
    return Roo::Constant::NIL;
  }

  std::optional<UI::Style::Text::Wrap> parse_text_wrap(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "word") return UI::Style::Text::Wrap::WORD;
    if (value->str() == "none") return UI::Style::Text::Wrap::NONE;
    return std::nullopt;
  }

  Roo::sptr_val text_wrap_to_value(const std::optional<UI::Style::Text::Wrap>& value)
  {
    if (!value) return Roo::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Text::Wrap::WORD:
      return Roo::keyword("word");
    case UI::Style::Text::Wrap::NONE:
      return Roo::keyword("none");
    }
    return Roo::Constant::NIL;
  }

  std::optional<UI::LayoutDirection> parse_layout_direction(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    return value->str() == "row" ? UI::LayoutDirection::ROW : UI::LayoutDirection::COLUMN;
  }

  Roo::sptr_val layout_direction_to_value(const std::optional<UI::LayoutDirection>& value)
  {
    if (!value) return Roo::Constant::NIL;
    return Roo::keyword(*value == UI::LayoutDirection::ROW ? "row" : "column");
  }

  std::optional<UI::Style::Layout::AlignItems> parse_layout_align_items(
    const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "start") return UI::Style::Layout::AlignItems::START;
    if (value->str() == "center") return UI::Style::Layout::AlignItems::CENTER;
    if (value->str() == "end") return UI::Style::Layout::AlignItems::END;
    return std::nullopt;
  }

  Roo::sptr_val layout_align_items_to_value(
    const std::optional<UI::Style::Layout::AlignItems>& value)
  {
    if (!value) return Roo::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Layout::AlignItems::START:
      return Roo::keyword("start");
    case UI::Style::Layout::AlignItems::CENTER:
      return Roo::keyword("center");
    case UI::Style::Layout::AlignItems::END:
      return Roo::keyword("end");
    }
    return Roo::Constant::NIL;
  }

  std::optional<UI::Style::Layout::GapMode> parse_layout_gap_mode(
    const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "none") return UI::Style::Layout::GapMode::NONE;
    if (value->str() == "fixed") return UI::Style::Layout::GapMode::FIXED;
    if (value->str() == "space-between") return UI::Style::Layout::GapMode::SPACE_BETWEEN;
    return std::nullopt;
  }

  Roo::sptr_val layout_gap_mode_to_value(
    const std::optional<UI::Style::Layout::GapMode>& value)
  {
    if (!value) return Roo::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Layout::GapMode::NONE:
      return Roo::keyword("none");
    case UI::Style::Layout::GapMode::FIXED:
      return Roo::keyword("fixed");
    case UI::Style::Layout::GapMode::SPACE_BETWEEN:
      return Roo::keyword("space-between");
    }
    return Roo::Constant::NIL;
  }

  std::optional<UI::Style::Layout::Wrap> parse_layout_wrap(
    const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "none") return UI::Style::Layout::Wrap::NONE;
    if (value->str() == "line") return UI::Style::Layout::Wrap::LINE;
    return std::nullopt;
  }

  Roo::sptr_val layout_wrap_to_value(
    const std::optional<UI::Style::Layout::Wrap>& value)
  {
    if (!value) return Roo::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Layout::Wrap::NONE:
      return Roo::keyword("none");
    case UI::Style::Layout::Wrap::LINE:
      return Roo::keyword("line");
    }
    return Roo::Constant::NIL;
  }

  std::optional<int> parse_optional_int(const Roo::sptr_val& value)
  {
    if (!value || value->type != Roo::Value::Type::NUMBER) return std::nullopt;
    return value->num().get_int();
  }

  Roo::sptr_val optional_int_to_value(const std::optional<int>& value)
  {
    return value ? Roo::number(*value) : Roo::Constant::NIL;
  }

  std::optional<bool> parse_optional_bool(const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;
    return Roo::is_truthy(*value);
  }

  Roo::sptr_val optional_bool_to_value(const std::optional<bool>& value)
  {
    if (!value) return Roo::Constant::NIL;
    return *value ? Roo::Constant::BOOL_TRUE : Roo::Constant::BOOL_FALSE;
  }

  std::unique_ptr<UI::Style::Layout::Gap> build_layout_gap(Roo::Context& ctx,
                                                           const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::STYLE_LAYOUT_GAP.is_type_of(*value))
    {
      return std::make_unique<UI::Style::Layout::Gap>(
        Roo::obj<UI::Style::Layout::Gap>(*value));
    }

    if (auto source = map_like_value(value))
    {
      static Roo::MapSchema gap_schema(
        {},
        {{"mode", &Roo::Type::KEYWORD}, {"size", &Roo::Type::NUMBER}});

      auto gap = std::make_unique<UI::Style::Layout::Gap>();
      auto opts = gap_schema.bind(ctx, *source);
      if (opts.contains("mode")) gap->mode = parse_layout_gap_mode(opts.val("mode"));
      if (opts.contains("size")) gap->size = opts.i32("size");
      return gap;
    }

    if (value->type == Roo::Value::Type::KEYWORD)
    {
      auto gap = std::make_unique<UI::Style::Layout::Gap>();
      gap->mode = parse_layout_gap_mode(value);
      return gap;
    }

    if (value->type == Roo::Value::Type::NUMBER)
    {
      auto gap = std::make_unique<UI::Style::Layout::Gap>();
      gap->mode = UI::Style::Layout::GapMode::FIXED;
      gap->size = value->num().get_int();
      return gap;
    }

    return nullptr;
  }

  std::unique_ptr<UI::Style::Text> build_text(Roo::Context& ctx,
                                              const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::STYLE_TEXT.is_type_of(*value))
    {
      return std::make_unique<UI::Style::Text>(Roo::obj<UI::Style::Text>(*value));
    }

    auto source = map_like_value(value);
    if (!source) return nullptr;

    static Roo::MapSchema text_schema({},
                                         {{"color", &Roo::Type::ANY},
                                          {"font", &Roo::Type::KEYWORD},
                                          {"scale", &Roo::Type::NUMBER},
                                          {"font-styles", &Roo::Type::ANY},
                                          {"align", &Roo::Type::KEYWORD},
                                          {"wrap", &Roo::Type::KEYWORD},
                                          {"shadow", &Roo::Type::ANY},
                                          {"marked-style", &Roo::Type::ANY}});

    auto text = std::make_unique<UI::Style::Text>();
    auto text_source = source;
    if (Roo::Dict::contains_key(*source, "color"))
    {
      auto color_value = Roo::Dict::get_property(*source, "color");

      if (parse_text_use_font_color(color_value))
      {
        text->use_font_color = true;
        text_source = Roo::Dict::shallow_copy(source);
        Roo::Dict::set_property(text_source,
                                   Roo::keyword("color"),
                                   Roo::Constant::NIL);
      }
    }

    auto opts = text_schema.bind(ctx, *text_source);
    text->color = optional_color(ctx, opts, "color");

    if (opts.contains("font"))
    {
      text->font = opts.str("font");
    }

    if (opts.contains("scale"))
    {
      text->scale = parse_text_scale(opts.val("scale"));
    }

    if (opts.contains("font-styles"))
    {
      text->font_styles = parse_text_font_styles(opts.val("font-styles"));
    }

    if (opts.contains("align"))
    {
      text->align = parse_text_align(opts.val("align"));
    }

    if (opts.contains("wrap"))
    {
      text->wrap = parse_text_wrap(opts.val("wrap"));
    }

    if (opts.contains("shadow"))
    {
      text->shadows = parse_text_shadows(ctx, opts.val("shadow"));
    }

    if (opts.contains("marked-style"))
    {
      text->marked_style = parse_marked_style(ctx, opts.val("marked-style"));
    }
    return text;
  }

  std::unique_ptr<UI::Style::Layout> build_layout(Roo::Context& ctx,
                                                  const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::STYLE_LAYOUT.is_type_of(*value))
    {
      return std::make_unique<UI::Style::Layout>(
        Roo::obj<UI::Style::Layout>(*value));
    }

    auto source = map_like_value(value);
    if (!source) return nullptr;

    static Roo::MapSchema layout_schema({},
                                           {{"direction", &Roo::Type::KEYWORD},
                                            {"align-items", &Roo::Type::KEYWORD},
                                            {"gap", &Roo::Type::ANY},
                                            {"wrap", &Roo::Type::KEYWORD},
                                            {"line-gap", &Roo::Type::NUMBER}});

    auto layout = std::make_unique<UI::Style::Layout>();
    auto opts = layout_schema.bind(ctx, *source);
    if (opts.contains("direction"))
    {
      layout->direction = parse_layout_direction(opts.val("direction"));
    }
    if (opts.contains("align-items"))
    {
      layout->align_items = parse_layout_align_items(opts.val("align-items"));
    }
    if (auto gap = build_layout_gap(ctx, opts.val("gap"))) layout->gap = *gap;
    if (opts.contains("wrap"))
    {
      layout->wrap = parse_layout_wrap(opts.val("wrap"));
    }
    if (opts.contains("line-gap"))
    {
      layout->line_gap = opts.i32("line-gap");
    }
    return layout;
  }

  std::unique_ptr<UI::Style::Image> build_image(Roo::Context& ctx,
                                                const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    auto source = map_like_value(value);
    if (!source) return nullptr;

    static Roo::MapSchema image_schema({},
                                       {{"opacity", &Roo::Type::NUMBER}});

    auto image = std::make_unique<UI::Style::Image>();
    auto opts = image_schema.bind(ctx, *source);
    image->opacity = parse_opacity(opts, "opacity");

    return image;
  }

  std::unique_ptr<UI::Style> build_style(Roo::Context& ctx, const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::STYLE.is_type_of(*value))
    {
      return std::make_unique<UI::Style>(Roo::obj<UI::Style>(*value));
    }

    auto source = map_like_value(value);
    if (!source) return nullptr;

    static Roo::MapSchema style_schema({},
                                          {{"background", &Roo::Type::ANY},
                                           {"image", &Roo::Type::ANY},
                                           {"margin", &Roo::Type::ANY},
                                           {"border", &Roo::Type::ANY},
                                           {"padding", &Roo::Type::ANY},
                                           {"corner-radius", &Roo::Type::ANY},
                                           {"layout", &Roo::Type::ANY},
                                           {"text", &Roo::Type::ANY},
                                           {"box-sizing", &Roo::Type::KEYWORD},
                                           {"scale", &Roo::Type::NUMBER},
                                           {"opacity", &Roo::Type::NUMBER},
                                           {"width", &Roo::Type::ANY},
                                           {"height", &Roo::Type::ANY},
                                           {"min-width", &Roo::Type::NUMBER},
                                           {"min-height", &Roo::Type::NUMBER},
                                           {"max-width", &Roo::Type::NUMBER},
                                           {"max-height", &Roo::Type::NUMBER},
                                           {"position", &Roo::Type::KEYWORD},
                                           {"top", &Roo::Type::NUMBER},
                                           {"left", &Roo::Type::NUMBER},
                                           {"visibility", &Roo::Type::KEYWORD},
                                           {"hidden", &Roo::Type::ANY},
                                           {"hit-test", &Roo::Type::ANY},
                                           {"clip", &Roo::Type::ANY},
                                           {"cursor", &Roo::Type::ANY},
                                           {"hover", &Roo::Type::ANY},
                                           {"focus-within", &Roo::Type::ANY},
                                           {"focus", &Roo::Type::ANY}});

    auto style = std::make_unique<UI::Style>();
    auto opts = style_schema.bind(ctx, *source);

    if (auto background = build_background(ctx, opts.val("background")))
      style->background = *background;
    if (auto image = build_image(ctx, opts.val("image"))) style->image = *image;
    if (auto margin = build_insets(ctx, opts.val("margin"))) style->margin = *margin;
    if (auto padding = build_insets(ctx, opts.val("padding"))) style->padding = *padding;
    if (opts.contains("corner-radius"))
    {
      style->corner_radius = parse_corner_radius(ctx, opts.val("corner-radius"));
    }
    if (auto border = build_border_style(ctx, opts.val("border"))) style->border = *border;
    if (auto layout = build_layout(ctx, opts.val("layout"))) style->layout = *layout;
    if (auto text = build_text(ctx, opts.val("text"))) style->text = *text;
    style->opacity = parse_opacity(opts, "opacity");
    if (opts.contains("box-sizing"))
    {
      style->box_sizing = parse_box_sizing(opts.val("box-sizing"));
    }

    if (opts.contains("scale")) style->scale = parse_scale(opts.val("scale"));
    if (opts.contains("width")) style->width = parse_size(opts.val("width"));
    if (opts.contains("height")) style->height = parse_size(opts.val("height"));
    if (opts.contains("min-width")) style->min_width = opts.i32("min-width");
    if (opts.contains("min-height")) style->min_height = opts.i32("min-height");
    if (opts.contains("max-width")) style->max_width = opts.i32("max-width");
    if (opts.contains("max-height")) style->max_height = opts.i32("max-height");
    if (opts.contains("position"))
    {
      style->position = parse_position_mode(opts.val("position"));
    }
    if (opts.contains("top")) style->top = opts.i32("top");
    if (opts.contains("left")) style->left = opts.i32("left");
    if (opts.contains("visibility"))
    {
      style->visibility = parse_visibility(opts.val("visibility"));
    }
    else if (opts.contains("hidden"))
    {
      style->visibility = parse_optional_bool(opts.val("hidden")).value_or(false)
                            ? UI::Style::Visibility::NONE
                            : UI::Style::Visibility::VISIBLE;
    }
    if (opts.contains("hit-test"))
    {
      style->hit_test = parse_optional_bool(opts.val("hit-test"));
    }
    if (opts.contains("clip")) style->clip = parse_optional_bool(opts.val("clip"));
    if (opts.contains("cursor")) style->cursor = parse_cursor(ctx, opts.val("cursor"));

    auto hover_style = build_style(ctx, opts.val("hover"));
    if (hover_style) style->hover = std::move(hover_style);

    auto focus_within_style = build_style(ctx, opts.val("focus-within"));
    if (focus_within_style)
    {
      style->focus_within = std::move(focus_within_style);
    }

    auto focus_style = build_style(ctx, opts.val("focus"));
    if (focus_style)
    {
      style->focus = std::move(focus_style);
    }

    return style;
  }

  std::unique_ptr<UI::Style::Background> build_background(Roo::Context& ctx,
                                                          const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::STYLE_BACKGROUND.is_type_of(*value))
    {
      return std::make_unique<UI::Style::Background>(
        Roo::obj<UI::Style::Background>(*value));
    }

    if (auto color = parse_color_value(ctx, value))
    {
      return std::make_unique<UI::Style::Background>(*color);
    }

    if (value->type == Roo::Value::Type::KEYWORD)
    {
      return std::make_unique<UI::Style::Background>(value->qual());
    }

    auto source = map_like_value(value);
    if (!source) return nullptr;

    static Roo::MapSchema background_schema({},
                                               {{"color", &Roo::Type::ANY},
                                                {"image", &Roo::Type::KEYWORD},
                                                {"source", &HostType::RECT},
                                                {"fit", &Roo::Type::KEYWORD},
                                                {"align", &Roo::Type::KEYWORD},
                                                {"offset", &HostType::POINT},
                                                {"opacity", &Roo::Type::NUMBER},
                                                {"repeat-x?", &Roo::Type::BOOL},
                                                {"repeat-y?", &Roo::Type::BOOL}});

    auto bg = std::make_unique<UI::Style::Background>();
    auto opts = background_schema.bind(ctx, *source);
    bg->color = optional_color(ctx, opts, "color");

    auto image_key = opts.val("image");
    if (image_key->type != Roo::Value::Type::NIL)
    {
      bg->image = image_key->qual();
    }
    bg->source = opts.optional_obj<Rect>("source");
    bg->fit = parse_background_fit(opts.val("fit"));
    apply_background_align(*bg, opts.val("align"));
    bg->offset = opts.optional_obj<Point>("offset");
    bg->opacity = parse_opacity(opts, "opacity");
    bg->repeat_x = parse_optional_bool(opts.val("repeat-x?"));
    bg->repeat_y = parse_optional_bool(opts.val("repeat-y?"));

    return bg;
  }

  std::unique_ptr<UI::Style::Border> build_border(Roo::Context& ctx,
                                                  const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::BORDER.is_type_of(*value))
    {
      return std::make_unique<UI::Style::Border>(
        Roo::obj<UI::Style::Border>(*value));
    }

    auto source = map_like_value(value);
    if (!source) return nullptr;

    static Roo::MapSchema border_schema({},
                                           {{"thickness", &Roo::Type::NUMBER},
                                            {"line-style", &Roo::Type::KEYWORD},
                                            {"color", &Roo::Type::ANY},
                                            {"trim", &Roo::Type::ANY}});

    auto border = std::make_unique<UI::Style::Border>();
    auto opts = border_schema.bind(ctx, *source);
    apply_border_props(ctx, *border, opts);
    return border;
  }

  std::unique_ptr<UI::Style::BorderStyle> build_border_style(Roo::Context& ctx,
                                                             const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::BORDER_STYLE.is_type_of(*value))
    {
      return std::make_unique<UI::Style::BorderStyle>(
        Roo::obj<UI::Style::BorderStyle>(*value));
    }

    auto source = map_like_value(value);
    if (!source) return nullptr;

    static Roo::MapSchema border_style_schema({},
                                                 {{"thickness", &Roo::Type::NUMBER},
                                                  {"line-style", &Roo::Type::KEYWORD},
                                                  {"color", &Roo::Type::ANY},
                                                  {"trim", &Roo::Type::ANY},
                                                  {"top", &Roo::Type::ANY},
                                                  {"right", &Roo::Type::ANY},
                                                  {"bottom", &Roo::Type::ANY},
                                                  {"left", &Roo::Type::ANY}});

    auto border = std::make_unique<UI::Style::BorderStyle>();
    auto opts = border_style_schema.bind(ctx, *source);
    apply_border_props(ctx, *border, opts);
    if (auto top = build_border(ctx, opts.val("top"))) border->t = *top;
    if (auto right = build_border(ctx, opts.val("right"))) border->r = *right;
    if (auto bottom = build_border(ctx, opts.val("bottom"))) border->b = *bottom;
    if (auto left = build_border(ctx, opts.val("left"))) border->l = *left;
    return border;
  }

  std::unique_ptr<UI::Style::Insets> build_insets(Roo::Context& ctx,
                                                  const Roo::sptr_val& value)
  {
    if (!value || value->type == Roo::Value::Type::NIL) return nullptr;

    if (HostType::STYLE_INSETS.is_type_of(*value))
    {
      return std::make_unique<UI::Style::Insets>(
        Roo::obj<UI::Style::Insets>(*value));
    }

    if (auto source = map_like_value(value))
    {
      static Roo::MapSchema insets_map_schema({},
                                                 {{"t", &Roo::Type::NUMBER},
                                                  {"r", &Roo::Type::NUMBER},
                                                  {"b", &Roo::Type::NUMBER},
                                                  {"l", &Roo::Type::NUMBER}});

      auto insets = std::make_unique<UI::Style::Insets>();
      auto opts = insets_map_schema.bind(ctx, *source);
      insets->t = opts.i32("t", 0);
      insets->r = opts.i32("r", 0);
      insets->b = opts.i32("b", 0);
      insets->l = opts.i32("l", 0);
      return insets;
    }

    if (value->type == Roo::Value::Type::NUMBER)
    {
      auto insets = std::make_unique<UI::Style::Insets>();
      int p = value->num().get_int();
      insets->t = p;
      insets->r = p;
      insets->b = p;
      insets->l = p;
      return insets;
    }

    if (value->type != Roo::Value::Type::VECTOR) return nullptr;

    int t = 0;
    int r = 0;
    int b = 0;
    int l = 0;

    switch (Roo::count(*value))
    {
    case 1:
      t = r = b = l = Roo::get_child(*value, 0)->num().get_int();
      break;
    case 2:
      t = b = Roo::get_child(*value, 0)->num().get_int();
      r = l = Roo::get_child(*value, 1)->num().get_int();
      break;
    case 4:
      t = Roo::get_child(*value, 0)->num().get_int();
      r = Roo::get_child(*value, 1)->num().get_int();
      b = Roo::get_child(*value, 2)->num().get_int();
      l = Roo::get_child(*value, 3)->num().get_int();
      break;
    default:
      return nullptr;
    }

    return std::make_unique<UI::Style::Insets>(t, r, b, l);
  }
} // namespace Pixils::Script::StyleDefinition
