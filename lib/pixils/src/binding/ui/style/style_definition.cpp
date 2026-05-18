#include "pixils/binding/ui/style/style_definition.h"

#include <pixils/binding/color_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/ui/style/style_host_type.h>

#include <algorithm>
#include <lisple/context.h>
#include <lisple/host/object.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/type.h>
#include <vector>

namespace Pixils::Script::StyleDefinition
{
  namespace
  {
    void apply_border_props(UI::Style::Border& border, Lisple::MapSchema::Inspector& opts)
    {
      if (opts.contains("thickness")) border.thickness = opts.i32("thickness");
      border.line_style = parse_line_style(opts.val("line-style"));
      border.color = opts.optional_obj<Color>("color");
      border.trim = parse_trim(opts.val("trim"));
    }

    std::optional<Pixils::Text::FontStyle> parse_font_style_keyword(
      const Lisple::sptr_val& value)
    {
      if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
      if (value->str() == "underline") return Pixils::Text::FontStyle::UNDERLINE;
      return std::nullopt;
    }

    std::optional<std::vector<Pixils::Text::FontStyle>> parse_text_font_styles(
      const Lisple::sptr_val& value)
    {
      if (!value || value->type == Lisple::Value::Type::NIL) return std::nullopt;

      if (auto single = parse_font_style_keyword(value))
      {
        return std::vector<Pixils::Text::FontStyle>{*single};
      }

      if (value->type != Lisple::Value::Type::VECTOR) return std::nullopt;

      std::vector<Pixils::Text::FontStyle> styles;
      for (auto& child : Lisple::get_children(*value))
      {
        auto parsed = parse_font_style_keyword(child);
        if (!parsed) return std::nullopt;
        styles.push_back(*parsed);
      }
      return styles;
    }

    std::optional<std::vector<Pixils::Text::Shadow>> parse_text_shadows(
      Lisple::Context& ctx,
      const Lisple::sptr_val& value)
    {
      if (!value || value->type == Lisple::Value::Type::NIL) return std::nullopt;

      static Lisple::MapSchema shadow_schema(
        {{"offset", &HostType::POINT}, {"color", &HostType::COLOR}},
        {});

      auto parse_one = [&](const Lisple::sptr_val& shadow_value)
      {
        auto sh = shadow_schema.bind(ctx, *shadow_value);
        return Pixils::Text::Shadow(sh.obj<Point>("offset"), sh.obj<Color>("color"));
      };

      std::vector<Pixils::Text::Shadow> shadows;
      if (value->type == Lisple::Value::Type::VECTOR)
      {
        for (auto& child : Lisple::get_children(*value))
        {
          shadows.push_back(parse_one(child));
        }
        return shadows;
      }

      if (value->type == Lisple::Value::Type::MAP)
      {
        shadows.push_back(parse_one(value));
        return shadows;
      }

      return std::nullopt;
    }

    std::optional<UI::Style::Background::Fit> parse_background_fit(
      const Lisple::sptr_val& value)
    {
      if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
      if (value->str() == "none") return UI::Style::Background::Fit::NONE;
      if (value->str() == "contain") return UI::Style::Background::Fit::CONTAIN;
      if (value->str() == "cover") return UI::Style::Background::Fit::COVER;
      if (value->str() == "fill") return UI::Style::Background::Fit::FILL;
      return std::nullopt;
    }

    std::optional<UI::Style::Background::Align> parse_background_align_keyword(
      const Lisple::sptr_val& value)
    {
      if (!value || value->type != Lisple::Value::Type::KEYWORD)
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

    void apply_background_align(UI::Style::Background& bg, const Lisple::sptr_val& value)
    {
      if (!value || value->type != Lisple::Value::Type::KEYWORD) return;

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

    std::optional<char> parse_inline_marker(const Lisple::sptr_val& value)
    {
      if (!value || value->type == Lisple::Value::Type::NIL) return std::nullopt;

      if (value->type == Lisple::Value::Type::CHAR)
      {
        return static_cast<char>(value->ch());
      }

      if (value->type == Lisple::Value::Type::STRING ||
          value->type == Lisple::Value::Type::KEYWORD ||
          value->type == Lisple::Value::Type::SYMBOL)
      {
        std::string raw = value->str();
        if (raw.size() == 1) return raw[0];
      }

      return std::nullopt;
    }

    std::optional<UI::Style::Text::MarkedStyle> parse_marked_style(
      Lisple::Context& ctx,
      const Lisple::sptr_val& value)
    {
      if (!value || value->type == Lisple::Value::Type::NIL) return std::nullopt;
      if (value->type != Lisple::Value::Type::MAP) return std::nullopt;

      static Lisple::MapSchema inline_schema({},
                                             {{"enabled", &Lisple::Type::BOOL},
                                              {"marker", &Lisple::Type::ANY},
                                              {"color", &HostType::COLOR},
                                              {"font", &Lisple::Type::KEYWORD},
                                              {"scale", &Lisple::Type::NUMBER},
                                              {"font-styles", &Lisple::Type::ANY},
                                              {"shadow", &Lisple::Type::ANY}});

      auto inline_source = value;
      if (Lisple::Dict::contains_key(*value, "color"))
      {
        auto color_value = Lisple::Dict::get_property(*value, "color");
        if (parse_text_use_font_color(color_value))
        {
          inline_source = Lisple::Dict::shallow_copy(value);
          Lisple::Dict::set_property(inline_source,
                                     Lisple::keyword("color"),
                                     Lisple::Constant::NIL);
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
        marked_style.color = opts.optional_obj<Color>("color");
      }

      if (opts.contains("font")) marked_style.font = opts.str("font");
      if (opts.contains("scale")) marked_style.scale = opts.i32("scale");
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

  std::optional<UI::Style::Size> parse_size(const Lisple::sptr_val& value)
  {
    if (!value || *value == *Lisple::Constant::NIL) return std::nullopt;

    switch (value->type)
    {
    case Lisple::Value::Type::NUMBER:
      return UI::Style::Size(value->num().get_int());
    case Lisple::Value::Type::KEYWORD:
      if (value->str() == "fill") return UI::Style::Size(UI::Style::Size::Mode::FILL);
      if (value->str() == "shrink") return UI::Style::Size(UI::Style::Size::Mode::SHRINK);
      if (value->str() == "auto") return UI::Style::Size(UI::Style::Size::Mode::AUTO);
      return std::nullopt;
    default:
      return std::nullopt;
    }
  }

  Lisple::sptr_val size_to_value(const std::optional<UI::Style::Size>& size)
  {
    if (!size) return Lisple::Constant::NIL;
    if (size->is_fixed()) return Lisple::number(size->fixed_value_or(0));
    if (size->is_fill()) return Lisple::keyword("fill");
    if (size->is_shrink()) return Lisple::keyword("shrink");
    return Lisple::keyword("auto");
  }

  std::optional<UI::Style::Trim> parse_trim(const Lisple::sptr_val& value)
  {
    if (!value || *value == *Lisple::Constant::NIL) return std::nullopt;

    switch (value->type)
    {
    case Lisple::Value::Type::NUMBER:
      return UI::Style::Trim{value->num().get_int()};
    case Lisple::Value::Type::VECTOR:
      switch (Lisple::count(*value))
      {
      case 1:
        return UI::Style::Trim{Lisple::get_child(*value, 0)->num().get_int()};
      case 2:
        return UI::Style::Trim{Lisple::get_child(*value, 0)->num().get_int(),
                               Lisple::get_child(*value, 1)->num().get_int()};
      default:
        return std::nullopt;
      }
    default:
      return std::nullopt;
    }
  }

  Lisple::sptr_val trim_to_value(const std::optional<UI::Style::Trim>& trim)
  {
    if (!trim) return Lisple::Constant::NIL;
    return Lisple::vector({Lisple::number(trim->start), Lisple::number(trim->end)});
  }

  std::optional<UI::PositionMode> parse_position_mode(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    return value->str() == "absolute" ? UI::PositionMode::ABSOLUTE : UI::PositionMode::FLOW;
  }

  Lisple::sptr_val position_mode_to_value(const std::optional<UI::PositionMode>& mode)
  {
    if (!mode) return Lisple::Constant::NIL;
    return Lisple::keyword(*mode == UI::PositionMode::ABSOLUTE ? "absolute" : "flow");
  }

  std::optional<float> parse_opacity(Lisple::MapSchema::Inspector& opts,
                                     const std::string& key)
  {
    if (!opts.contains(key)) return std::nullopt;
    return std::clamp(opts.f32(key), 0.0f, 1.0f);
  }

  std::optional<UI::Style::BoxSizing> parse_box_sizing(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    return value->str() == "content-box" ? UI::Style::BoxSizing::CONTENT_BOX
                                         : UI::Style::BoxSizing::BORDER_BOX;
  }

  Lisple::sptr_val box_sizing_to_value(const std::optional<UI::Style::BoxSizing>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    return Lisple::keyword(*value == UI::Style::BoxSizing::CONTENT_BOX ? "content-box"
                                                                       : "border-box");
  }

  std::optional<UI::Style::Visibility> parse_visibility(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "visible") return UI::Style::Visibility::VISIBLE;
    if (value->str() == "hidden") return UI::Style::Visibility::HIDDEN;
    if (value->str() == "none") return UI::Style::Visibility::NONE;
    return std::nullopt;
  }

  Lisple::sptr_val visibility_to_value(const std::optional<UI::Style::Visibility>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Visibility::VISIBLE:
      return Lisple::keyword("visible");
    case UI::Style::Visibility::HIDDEN:
      return Lisple::keyword("hidden");
    case UI::Style::Visibility::NONE:
      return Lisple::keyword("none");
    }
    return Lisple::Constant::NIL;
  }

  std::optional<int> parse_scale(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::NUMBER) return std::nullopt;
    return std::max(1, value->num().get_int());
  }

  Lisple::sptr_val scale_to_value(const std::optional<int>& value)
  {
    return value ? Lisple::number(*value) : Lisple::Constant::NIL;
  }

  std::optional<UI::SystemCursor> parse_system_cursor(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
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

  std::optional<UI::ImageCursor> parse_image_cursor(Lisple::Context& ctx,
                                                    const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::MAP) return std::nullopt;

    static Lisple::MapSchema pointer_schema({{"image", &Lisple::Type::KEYWORD}},
                                            {{"source", &HostType::RECT},
                                             {"hotspot", &HostType::POINT},
                                             {"scale", &Lisple::Type::NUMBER},
                                             {"render", &Lisple::Type::KEYWORD}});

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

  std::optional<UI::CursorSpec> parse_cursor(Lisple::Context& ctx,
                                             const Lisple::sptr_val& value)
  {
    if (!value || *value == *Lisple::Constant::NIL) return std::nullopt;

    if (auto system = parse_system_cursor(value))
    {
      return UI::CursorSpec::system_cursor(*system);
    }

    if (value->type == Lisple::Value::Type::KEYWORD)
    {
      return UI::CursorSpec::named(value->str());
    }

    if (auto image_cursor = parse_image_cursor(ctx, value))
    {
      return UI::CursorSpec::image_cursor(*image_cursor);
    }

    return std::nullopt;
  }

  Lisple::sptr_val system_cursor_to_value(const UI::SystemCursor& value)
  {
    switch (value)
    {
    case UI::SystemCursor::DEFAULT:
      return Lisple::keyword("default");
    case UI::SystemCursor::POINTER:
      return Lisple::keyword("pointer");
    case UI::SystemCursor::TEXT:
      return Lisple::keyword("text");
    case UI::SystemCursor::CROSSHAIR:
      return Lisple::keyword("crosshair");
    case UI::SystemCursor::MOVE:
      return Lisple::keyword("move");
    case UI::SystemCursor::NOT_ALLOWED:
      return Lisple::keyword("not-allowed");
    case UI::SystemCursor::WAIT:
      return Lisple::keyword("wait");
    case UI::SystemCursor::PROGRESS:
      return Lisple::keyword("progress");
    case UI::SystemCursor::RESIZE_X:
      return Lisple::keyword("resize-x");
    case UI::SystemCursor::RESIZE_Y:
      return Lisple::keyword("resize-y");
    case UI::SystemCursor::RESIZE_NWSE:
      return Lisple::keyword("resize-nwse");
    case UI::SystemCursor::RESIZE_NESW:
      return Lisple::keyword("resize-nesw");
    }
    return Lisple::Constant::NIL;
  }

  Lisple::sptr_val cursor_to_value(const std::optional<UI::CursorSpec>& value)
  {
    if (!value) return Lisple::Constant::NIL;

    switch (value->kind)
    {
    case UI::CursorSpec::Kind::SYSTEM:
      return system_cursor_to_value(value->system);
    case UI::CursorSpec::Kind::NAMED:
      return Lisple::keyword(value->name);
    case UI::CursorSpec::Kind::IMAGE:
    {
      std::vector<Lisple::sptr_val> values;
      if (value->image.image)
      {
        values.push_back(Lisple::keyword("image"));
        values.push_back(
          Lisple::keyword(value->image.image->first + "/" + value->image.image->second));
      }
      if (value->image.source)
      {
        values.push_back(Lisple::keyword("source"));
        values.push_back(Lisple::map({Lisple::keyword("x"),
                                      Lisple::number(value->image.source->x),
                                      Lisple::keyword("y"),
                                      Lisple::number(value->image.source->y),
                                      Lisple::keyword("w"),
                                      Lisple::number(value->image.source->w),
                                      Lisple::keyword("h"),
                                      Lisple::number(value->image.source->h)}));
      }
      values.push_back(Lisple::keyword("hotspot"));
      values.push_back(Lisple::map({Lisple::keyword("x"),
                                    Lisple::number(value->image.hotspot.round_x()),
                                    Lisple::keyword("y"),
                                    Lisple::number(value->image.hotspot.round_y())}));
      values.push_back(Lisple::keyword("scale"));
      values.push_back(Lisple::number(value->image.scale));
      values.push_back(Lisple::keyword("render"));
      values.push_back(Lisple::keyword(
        value->image.render_mode == UI::ImageCursor::RenderMode::NATIVE ? "native" : "app"));
      return Lisple::map(values);
    }
    }

    return Lisple::Constant::NIL;
  }

  std::optional<UI::Style::LineStyle> parse_line_style(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "solid") return UI::Style::LineStyle::SOLID;
    if (value->str() == "bevel") return UI::Style::LineStyle::BEVEL;
    return std::nullopt;
  }

  Lisple::sptr_val line_style_to_value(const std::optional<UI::Style::LineStyle>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    switch (*value)
    {
    case UI::Style::LineStyle::SOLID:
      return Lisple::keyword("solid");
    case UI::Style::LineStyle::BEVEL:
      return Lisple::keyword("bevel");
    }
    return Lisple::Constant::NIL;
  }

  std::optional<Pixils::Text::Alignment> parse_text_align(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "left") return Pixils::Text::Alignment::LEFT;
    if (value->str() == "center") return Pixils::Text::Alignment::CENTER;
    if (value->str() == "right") return Pixils::Text::Alignment::RIGHT;
    return std::nullopt;
  }

  Lisple::sptr_val text_align_to_value(const std::optional<Pixils::Text::Alignment>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    switch (*value)
    {
    case Pixils::Text::Alignment::LEFT:
      return Lisple::keyword("left");
    case Pixils::Text::Alignment::CENTER:
      return Lisple::keyword("center");
    case Pixils::Text::Alignment::RIGHT:
      return Lisple::keyword("right");
    }
    return Lisple::Constant::NIL;
  }

  bool parse_text_use_font_color(const Lisple::sptr_val& value)
  {
    return value && value->type == Lisple::Value::Type::KEYWORD && value->str() == "none";
  }

  Lisple::sptr_val text_color_to_value(const UI::Style::Text& text)
  {
    if (text.use_font_color) return Lisple::keyword("none");
    if (text.color) return ColorAdapter::make_ref(*text.color);
    return Lisple::Constant::NIL;
  }

  std::optional<UI::Style::Text::Wrap> parse_text_wrap(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "word") return UI::Style::Text::Wrap::WORD;
    if (value->str() == "none") return UI::Style::Text::Wrap::NONE;
    return std::nullopt;
  }

  Lisple::sptr_val text_wrap_to_value(const std::optional<UI::Style::Text::Wrap>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Text::Wrap::WORD:
      return Lisple::keyword("word");
    case UI::Style::Text::Wrap::NONE:
      return Lisple::keyword("none");
    }
    return Lisple::Constant::NIL;
  }

  std::optional<UI::LayoutDirection> parse_layout_direction(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    return value->str() == "row" ? UI::LayoutDirection::ROW : UI::LayoutDirection::COLUMN;
  }

  Lisple::sptr_val layout_direction_to_value(const std::optional<UI::LayoutDirection>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    return Lisple::keyword(*value == UI::LayoutDirection::ROW ? "row" : "column");
  }

  std::optional<UI::Style::Layout::AlignItems> parse_layout_align_items(
    const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "start") return UI::Style::Layout::AlignItems::START;
    if (value->str() == "center") return UI::Style::Layout::AlignItems::CENTER;
    if (value->str() == "end") return UI::Style::Layout::AlignItems::END;
    return std::nullopt;
  }

  Lisple::sptr_val layout_align_items_to_value(
    const std::optional<UI::Style::Layout::AlignItems>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Layout::AlignItems::START:
      return Lisple::keyword("start");
    case UI::Style::Layout::AlignItems::CENTER:
      return Lisple::keyword("center");
    case UI::Style::Layout::AlignItems::END:
      return Lisple::keyword("end");
    }
    return Lisple::Constant::NIL;
  }

  std::optional<UI::Style::Layout::GapMode> parse_layout_gap_mode(
    const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
    if (value->str() == "none") return UI::Style::Layout::GapMode::NONE;
    if (value->str() == "fixed") return UI::Style::Layout::GapMode::FIXED;
    if (value->str() == "space-between") return UI::Style::Layout::GapMode::SPACE_BETWEEN;
    return std::nullopt;
  }

  Lisple::sptr_val layout_gap_mode_to_value(
    const std::optional<UI::Style::Layout::GapMode>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    switch (*value)
    {
    case UI::Style::Layout::GapMode::NONE:
      return Lisple::keyword("none");
    case UI::Style::Layout::GapMode::FIXED:
      return Lisple::keyword("fixed");
    case UI::Style::Layout::GapMode::SPACE_BETWEEN:
      return Lisple::keyword("space-between");
    }
    return Lisple::Constant::NIL;
  }

  std::optional<int> parse_optional_int(const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::NUMBER) return std::nullopt;
    return value->num().get_int();
  }

  Lisple::sptr_val optional_int_to_value(const std::optional<int>& value)
  {
    return value ? Lisple::number(*value) : Lisple::Constant::NIL;
  }

  std::optional<bool> parse_optional_bool(const Lisple::sptr_val& value)
  {
    if (!value || *value == *Lisple::Constant::NIL) return std::nullopt;
    return Lisple::is_truthy(*value);
  }

  Lisple::sptr_val optional_bool_to_value(const std::optional<bool>& value)
  {
    if (!value) return Lisple::Constant::NIL;
    return *value ? Lisple::Constant::BOOL_TRUE : Lisple::Constant::BOOL_FALSE;
  }

  std::unique_ptr<UI::Style::Layout::Gap> build_layout_gap(Lisple::Context& ctx,
                                                           const Lisple::sptr_val& value)
  {
    if (!value || *value == *Lisple::Constant::NIL) return nullptr;

    if (value->type == Lisple::Value::Type::MAP)
    {
      static Lisple::MapSchema gap_schema(
        {},
        {{"mode", &Lisple::Type::KEYWORD}, {"size", &Lisple::Type::NUMBER}});

      auto gap = std::make_unique<UI::Style::Layout::Gap>();
      auto opts = gap_schema.bind(ctx, *value);
      if (opts.contains("mode")) gap->mode = parse_layout_gap_mode(opts.val("mode"));
      if (opts.contains("size")) gap->size = opts.i32("size");
      return gap;
    }

    if (value->type == Lisple::Value::Type::KEYWORD)
    {
      auto gap = std::make_unique<UI::Style::Layout::Gap>();
      gap->mode = parse_layout_gap_mode(value);
      return gap;
    }

    if (value->type == Lisple::Value::Type::NUMBER)
    {
      auto gap = std::make_unique<UI::Style::Layout::Gap>();
      gap->mode = UI::Style::Layout::GapMode::FIXED;
      gap->size = value->num().get_int();
      return gap;
    }

    return nullptr;
  }

  std::unique_ptr<UI::Style::Text> build_text(Lisple::Context& ctx,
                                              const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::MAP) return nullptr;

    static Lisple::MapSchema text_schema({},
                                         {{"color", &HostType::COLOR},
                                          {"font", &Lisple::Type::KEYWORD},
                                          {"scale", &Lisple::Type::NUMBER},
                                          {"font-styles", &Lisple::Type::ANY},
                                          {"align", &Lisple::Type::KEYWORD},
                                          {"wrap", &Lisple::Type::KEYWORD},
                                          {"shadow", &Lisple::Type::ANY},
                                          {"marked-style", &Lisple::Type::ANY}});

    auto text = std::make_unique<UI::Style::Text>();
    auto text_source = value;
    if (Lisple::Dict::contains_key(*value, "color"))
    {
      auto color_value = Lisple::Dict::get_property(*value, "color");

      if (parse_text_use_font_color(color_value))
      {
        text->use_font_color = true;
        text_source = Lisple::Dict::shallow_copy(value);
        Lisple::Dict::set_property(text_source,
                                   Lisple::keyword("color"),
                                   Lisple::Constant::NIL);
      }
    }

    auto opts = text_schema.bind(ctx, *text_source);
    text->color = opts.optional_obj<Color>("color");

    if (opts.contains("font"))
    {
      text->font = opts.str("font");
    }

    if (opts.contains("scale"))
    {
      text->scale = opts.i32("scale");
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

  std::unique_ptr<UI::Style::Layout> build_layout(Lisple::Context& ctx,
                                                  const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::MAP) return nullptr;

    static Lisple::MapSchema layout_schema({},
                                           {{"direction", &Lisple::Type::KEYWORD},
                                            {"align-items", &Lisple::Type::KEYWORD},
                                            {"gap", &HostType::STYLE_LAYOUT_GAP}});

    auto layout = std::make_unique<UI::Style::Layout>();
    auto opts = layout_schema.bind(ctx, *value);
    if (opts.contains("direction"))
    {
      layout->direction = parse_layout_direction(opts.val("direction"));
    }
    if (opts.contains("align-items"))
    {
      layout->align_items = parse_layout_align_items(opts.val("align-items"));
    }
    layout->gap = opts.optional_obj<UI::Style::Layout::Gap>("gap");
    return layout;
  }

  std::unique_ptr<UI::Style> build_style(Lisple::Context& ctx, const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::MAP) return nullptr;

    static Lisple::MapSchema style_schema({},
                                          {{"background", &HostType::STYLE_BACKGROUND},
                                           {"margin", &HostType::STYLE_INSETS},
                                           {"border", &HostType::BORDER_STYLE},
                                           {"padding", &HostType::STYLE_INSETS},
                                           {"layout", &HostType::STYLE_LAYOUT},
                                           {"text", &HostType::STYLE_TEXT},
                                           {"box-sizing", &Lisple::Type::KEYWORD},
                                           {"scale", &Lisple::Type::NUMBER},
                                           {"opacity", &Lisple::Type::NUMBER},
                                           {"width", &Lisple::Type::ANY},
                                           {"height", &Lisple::Type::ANY},
                                           {"position", &Lisple::Type::KEYWORD},
                                           {"top", &Lisple::Type::NUMBER},
                                           {"left", &Lisple::Type::NUMBER},
                                           {"visibility", &Lisple::Type::KEYWORD},
                                           {"hidden", &Lisple::Type::ANY},
                                           {"hit-test", &Lisple::Type::ANY},
                                           {"clip", &Lisple::Type::ANY},
                                           {"cursor", &Lisple::Type::ANY},
                                           {"hover", &HostType::STYLE},
                                           {"focus-within", &HostType::STYLE},
                                           {"focus", &HostType::STYLE}});

    auto style = std::make_unique<UI::Style>();
    auto opts = style_schema.bind(ctx, *value);

    style->background = opts.optional_obj<UI::Style::Background>("background");
    style->margin = opts.optional_obj<UI::Style::Insets>("margin");
    style->padding = opts.optional_obj<UI::Style::Insets>("padding");
    style->border = opts.optional_obj<UI::Style::BorderStyle>("border");
    style->layout = opts.optional_obj<UI::Style::Layout>("layout");
    style->text = opts.optional_obj<UI::Style::Text>("text");
    style->opacity = parse_opacity(opts, "opacity");
    if (opts.contains("box-sizing"))
    {
      style->box_sizing = parse_box_sizing(opts.val("box-sizing"));
    }

    if (opts.contains("scale")) style->scale = parse_scale(opts.val("scale"));
    if (opts.contains("width")) style->width = parse_size(opts.val("width"));
    if (opts.contains("height")) style->height = parse_size(opts.val("height"));
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

    auto hover_style = opts.optional_obj<UI::Style>("hover");
    if (hover_style) style->hover = std::make_unique<UI::Style>(*hover_style);

    auto focus_within_style = opts.optional_obj<UI::Style>("focus-within");
    if (focus_within_style)
    {
      style->focus_within = std::make_unique<UI::Style>(*focus_within_style);
    }

    auto focus_style = opts.optional_obj<UI::Style>("focus");
    if (focus_style)
    {
      style->focus = std::make_unique<UI::Style>(*focus_style);
    }

    return style;
  }

  std::unique_ptr<UI::Style::Background> build_background(Lisple::Context& ctx,
                                                          const Lisple::sptr_val& value)
  {
    if (!value || *value == *Lisple::Constant::NIL) return nullptr;

    if (HostType::COLOR.is_type_of(*value))
    {
      return std::make_unique<UI::Style::Background>(Lisple::obj<Color>(*value));
    }

    if (value->type == Lisple::Value::Type::KEYWORD)
    {
      return std::make_unique<UI::Style::Background>(value->qual());
    }

    if (value->type != Lisple::Value::Type::MAP) return nullptr;

    if (Lisple::Dict::contains_key(*value, "r"))
    {
      auto color_value = value;
      auto coercion = HostType::COLOR.coerce(ctx, color_value);
      if (coercion.success)
      {
        return std::make_unique<UI::Style::Background>(Lisple::obj<Color>(*coercion.result));
      }
    }

    static Lisple::MapSchema background_schema({},
                                               {{"color", &HostType::COLOR},
                                                {"image", &Lisple::Type::KEYWORD},
                                                {"source", &HostType::RECT},
                                                {"fit", &Lisple::Type::KEYWORD},
                                                {"align", &Lisple::Type::KEYWORD},
                                                {"offset", &HostType::POINT},
                                                {"opacity", &Lisple::Type::NUMBER}});

    auto bg = std::make_unique<UI::Style::Background>();
    auto opts = background_schema.bind(ctx, *value);
    bg->color = opts.optional_obj<Color>("color");

    auto image_key = opts.val("image");
    if (image_key->type != Lisple::Value::Type::NIL)
    {
      bg->image = image_key->qual();
    }
    bg->source = opts.optional_obj<Rect>("source");
    bg->fit = parse_background_fit(opts.val("fit"));
    apply_background_align(*bg, opts.val("align"));
    bg->offset = opts.optional_obj<Point>("offset");
    bg->opacity = parse_opacity(opts, "opacity");

    return bg;
  }

  std::unique_ptr<UI::Style::Border> build_border(Lisple::Context& ctx,
                                                  const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::MAP) return nullptr;

    static Lisple::MapSchema border_schema({},
                                           {{"thickness", &Lisple::Type::NUMBER},
                                            {"line-style", &Lisple::Type::KEYWORD},
                                            {"color", &HostType::COLOR},
                                            {"trim", &Lisple::Type::ANY}});

    auto border = std::make_unique<UI::Style::Border>();
    auto opts = border_schema.bind(ctx, *value);
    apply_border_props(*border, opts);
    return border;
  }

  std::unique_ptr<UI::Style::BorderStyle> build_border_style(Lisple::Context& ctx,
                                                             const Lisple::sptr_val& value)
  {
    if (!value || value->type != Lisple::Value::Type::MAP) return nullptr;

    static Lisple::MapSchema border_style_schema({},
                                                 {{"thickness", &Lisple::Type::NUMBER},
                                                  {"line-style", &Lisple::Type::KEYWORD},
                                                  {"color", &HostType::COLOR},
                                                  {"trim", &Lisple::Type::ANY},
                                                  {"top", &HostType::BORDER},
                                                  {"right", &HostType::BORDER},
                                                  {"bottom", &HostType::BORDER},
                                                  {"left", &HostType::BORDER}});

    auto border = std::make_unique<UI::Style::BorderStyle>();
    auto opts = border_style_schema.bind(ctx, *value);
    apply_border_props(*border, opts);
    border->t = opts.optional_obj<UI::Style::Border>("top");
    border->r = opts.optional_obj<UI::Style::Border>("right");
    border->b = opts.optional_obj<UI::Style::Border>("bottom");
    border->l = opts.optional_obj<UI::Style::Border>("left");
    return border;
  }

  std::unique_ptr<UI::Style::Insets> build_insets(Lisple::Context& ctx,
                                                  const Lisple::sptr_val& value)
  {
    if (!value || *value == *Lisple::Constant::NIL) return nullptr;

    if (value->type == Lisple::Value::Type::MAP)
    {
      static Lisple::MapSchema insets_map_schema({},
                                                 {{"t", &Lisple::Type::NUMBER},
                                                  {"r", &Lisple::Type::NUMBER},
                                                  {"b", &Lisple::Type::NUMBER},
                                                  {"l", &Lisple::Type::NUMBER}});

      auto insets = std::make_unique<UI::Style::Insets>();
      auto opts = insets_map_schema.bind(ctx, *value);
      insets->t = opts.i32("t", 0);
      insets->r = opts.i32("r", 0);
      insets->b = opts.i32("b", 0);
      insets->l = opts.i32("l", 0);
      return insets;
    }

    if (value->type == Lisple::Value::Type::NUMBER)
    {
      auto insets = std::make_unique<UI::Style::Insets>();
      int p = value->num().get_int();
      insets->t = p;
      insets->r = p;
      insets->b = p;
      insets->l = p;
      return insets;
    }

    if (value->type != Lisple::Value::Type::VECTOR) return nullptr;

    int t = 0;
    int r = 0;
    int b = 0;
    int l = 0;

    switch (Lisple::count(*value))
    {
    case 1:
      t = r = b = l = Lisple::get_child(*value, 0)->num().get_int();
      break;
    case 2:
      t = b = Lisple::get_child(*value, 0)->num().get_int();
      r = l = Lisple::get_child(*value, 1)->num().get_int();
      break;
    case 4:
      t = Lisple::get_child(*value, 0)->num().get_int();
      r = Lisple::get_child(*value, 1)->num().get_int();
      b = Lisple::get_child(*value, 2)->num().get_int();
      l = Lisple::get_child(*value, 3)->num().get_int();
      break;
    default:
      return nullptr;
    }

    return std::make_unique<UI::Style::Insets>(t, r, b, l);
  }
} // namespace Pixils::Script::StyleDefinition
