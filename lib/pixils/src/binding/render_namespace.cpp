
#include "pixils/binding/render_namespace.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>
#include <pixils/geom.h>

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_render.h>
#include <lisple/host/schema.h>
#include <lisple/namespace.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

#include <algorithm>
#include <cmath>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(CLOSE, "close");
    SHKEY(COLOR, "color");
    SHKEY(FILL, "fill");
    SHKEY(OFFSET, "offset");
    SHKEY(POS, "pos");
    SHKEY(ROTATION, "rotation");
    SHKEY(SCALE, "scale");
    SHKEY(SOURCE, "source");
    SHKEY(OPACITY, "opacity");
  } // namespace MapKey

  namespace Function
  {
    namespace
    {
      constexpr double RADIANS_TO_DEGREES = 180.0 / 3.14159265358979323846;

      Uint8 opacity_to_alpha(float opacity)
      {
        return static_cast<Uint8>(
          std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
      }

      Uint8 image_opacity_alpha(Lisple::MapSchema::Inspector& opts)
      {
        if (opts.contains(MapKey::OPACITY->value))
        {
          return opacity_to_alpha(opts.f32(MapKey::OPACITY->value));
        }
        return 255;
      }
    }

    FUNC_IMPL(DrawImageBang,
              MULTI_SIG((FN_ARGS((&Lisple::Type::KEY), (&HostType::POINT)),
                         EXEC_DISPATCH(&DrawImageBang::exec_draw_img)),
                        (FN_ARGS((&Lisple::Type::KEY), (&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&DrawImageBang::exec_draw_img))));

    EXEC_BODY(DrawImageBang, exec_draw_img)
    {
      static Lisple::MapSchema draw_image_opts_schema({{"pos", &HostType::POINT}},
                                                      {{"scale", &Lisple::Type::NUMBER},
                                                       {"opacity", &Lisple::Type::NUMBER},
                                                       {"rotation", &Lisple::Type::NUMBER},
                                                       {"source", &HostType::RECT}});

      if (args[0]->type == Lisple::RTValue::Type::KEYWORD)
      {
        auto [asset_bundle, asset_key] = args.front()->qual();

        /**
         * Force detection of Point-arg, as coercion will not have happened during
         * for map-shaped Points during dispatch
         */
        Lisple::sptr_rtval map_arg =
          Lisple::Dict::contains_key(*args[1], "pos")
            ? args[1]
            : Lisple::RTValue::map({Lisple::RTValue::keyword("pos"), args[1]});

        auto opts = draw_image_opts_schema.bind(ctx, *map_arg);

        RenderContext& rc =
          Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

        SDL_Texture* texture = rc.asset_registry->get_image(asset_bundle, asset_key);

        Point& pos = opts.obj<Point>("pos");
        float scale = opts.f32("scale", 1.0f);
        float rotation = opts.f32(MapKey::ROTATION->value, 0.0f);
        Uint8 alpha = image_opacity_alpha(opts);
        std::optional<SDL_Rect> source_rect = std::nullopt;
        if (auto source = opts.val(MapKey::SOURCE->value);
            source && source->type != Lisple::RTValue::Type::NIL)
        {
          source_rect = opts.obj<Rect>(MapKey::SOURCE->value).to_SDL_rect();
        }

        SDL_Rect dim{pos.round_x(), pos.round_y(), 0, 0};
        SDL_QueryTexture(texture, nullptr, nullptr, &dim.w, &dim.h);
        if (source_rect)
        {
          dim.w = source_rect->w;
          dim.h = source_rect->h;
        }

        dim.w *= scale;
        dim.h *= scale;
        const SDL_Rect* source_ptr = source_rect ? &*source_rect : nullptr;

        if (texture)
        {
          SDL_SetTextureAlphaMod(texture, alpha);
          if (rotation == 0.0f)
          {
            SDL_RenderCopy(rc.renderer, texture, source_ptr, &dim);
          }
          else
          {
            SDL_RenderCopyEx(rc.renderer,
                             texture,
                             source_ptr,
                             &dim,
                             static_cast<double>(rotation) * RADIANS_TO_DEGREES,
                             nullptr,
                             SDL_FLIP_NONE);
          }
          if (alpha != 255) SDL_SetTextureAlphaMod(texture, 255);
        }
      }

      return Lisple::Constant::NIL;
    }

    /* DrawLineBang - line! */
    FUNC_IMPL(DrawLineBang,
              MULTI_SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                         EXEC_DISPATCH(&DrawLineBang::exec_draw_line)),
                        (FN_ARGS((&HostType::POINT), (&HostType::POINT), (&HostType::COLOR)),
                         EXEC_DISPATCH(&DrawLineBang::exec_draw_line))));

    EXEC_BODY(DrawLineBang, exec_draw_line)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      const Point& from = Lisple::obj<Point>(*args[0]);
      const Point& to = Lisple::obj<Point>(*args[1]);

      if (args.size() == 3 && Lisple::is_truthy(*args[2]))
      {
        const Color& color = Lisple::obj<Color>(*args[2]);
        SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
      }

      SDL_RenderDrawLine(rc.renderer,
                         from.round_x(),
                         from.round_y(),
                         to.round_x(),
                         to.round_y());

      return Lisple::Constant::NIL;
    }

    /* DrawPolygonbang - polygon! */
    FUNC_IMPL(DrawPolygonBang,
              MULTI_SIG((FN_ARGS((&HostType::VECTOR_OF_POINT)),
                         EXEC_DISPATCH(&DrawPolygonBang::exec_polygon)),
                        (FN_ARGS((&HostType::VECTOR_OF_POINT), (&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&DrawPolygonBang::exec_polygon_with_opts))));

    Lisple::MapSchema polygon_opts({},
                                   {{MapKey::CLOSE->value, &Lisple::Type::BOOL},
                                    {MapKey::ROTATION->value, &Lisple::Type::NUMBER},
                                    {MapKey::OFFSET->value, &HostType::POINT},
                                    {MapKey::COLOR->value, &HostType::COLOR},
                                    {MapKey::SCALE->value, &Lisple::Type::NUMBER}});

    EXEC_BODY(DrawPolygonBang, exec_polygon)
    {
      Lisple::sptr_rtval_v opt_args = args;
      opt_args.push_back(Lisple::RTValue::map({}));
      return this->exec_polygon_with_opts(ctx, opt_args);
    }

    EXEC_BODY(DrawPolygonBang, exec_polygon_with_opts)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      auto opts = polygon_opts.bind(ctx, *args.back());

      Lisple::sptr_rtval& polygon = args.front();
      Lisple::sptr_rtval_v points = Lisple::get_children(*polygon);

      bool close_shape = opts.boolean(MapKey::CLOSE->value, false);

      float rotation = opts.f32(MapKey::ROTATION->value, 0.0f);
      float scale = opts.f32(MapKey::SCALE->value, 1.0f);
      std::optional<Color> color = opts.optional_obj<Color>(MapKey::COLOR->value);

      const Point& offset = opts.obj<Point>(MapKey::OFFSET->value, POINT__ZERO_ZERO);

      if (color)
      {
        SDL_SetRenderDrawColor(rc.renderer, color->r, color->g, color->b, color->a);
      }

      std::vector<Point> pts;
      if (points.size() > 0)
      {
        pts.reserve(points.size());
        for (auto& poly_pt : points)
        {
          pts.push_back((Lisple::obj<Point>(*poly_pt) * scale)
                          .rotate(POINT__ZERO_ZERO, rotation)
                          .plus(offset.x, offset.y));
        }

        if (close_shape)
        {
          pts.push_back((Lisple::obj<Point>(*points.front()) * scale)
                          .rotate(POINT__ZERO_ZERO, rotation)
                          .plus(offset.x, offset.y));
        }

        for (size_t i = 0; i < pts.size() - 1; i++)
        {
          const Point& from = pts[i];
          const Point& to = pts[i + 1];
          SDL_RenderDrawLine(rc.renderer,
                             from.round_x(),
                             from.round_y(),
                             to.round_x(),
                             to.round_y());
        }
      }

      return Lisple::Constant::NIL;
    }

    /* DrawRectBang - rect! */
    FUNC_IMPL(
      DrawRectBang,
      MULTI_SIG((FN_ARGS((&HostType::RECT), (&Lisple::Type::MAP)),
                 EXEC_DISPATCH(&DrawRectBang::exec_draw_rect)),
                (FN_ARGS((&HostType::POINT), (&HostType::POINT), (&Lisple::Type::MAP)),
                 EXEC_DISPATCH(&DrawRectBang::exec_draw_rect_from_points))));

    EXEC_BODY(DrawRectBang, exec_draw_rect)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const Rect& hrect = Lisple::obj<Rect>(*args[0]);
      const Point top_left = hrect.top_left();
      const Point bottom_right = hrect.bottom_right();

      static Lisple::MapSchema opts_schema(
        {},
        {{"color", &HostType::COLOR}, {"fill", &Lisple::Type::BOOL}});

      auto opts = opts_schema.bind(ctx, *args[1]);

      auto color_opt = opts.val("color");
      auto fill_opt = opts.val("fill");

      if (Lisple::is_truthy(*color_opt))
      {
        const Color& color = Lisple::obj<Color>(*color_opt);
        SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
      }

      const Point wh = bottom_right - top_left;

      SDL_Rect rect = {top_left.round_x(), top_left.round_y(), wh.round_x(), wh.round_y()};

      if (Lisple::is_truthy(*fill_opt))
      {
        SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(rc.renderer, &rect);
        SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);
      }
      else
      {
        SDL_RenderDrawLine(rc.renderer,
                           top_left.round_x(),
                           top_left.round_y(),
                           bottom_right.round_x(),
                           top_left.round_y());
        SDL_RenderDrawLine(rc.renderer,
                           top_left.round_x(),
                           bottom_right.round_y(),
                           bottom_right.round_x(),
                           bottom_right.round_y());
        SDL_RenderDrawLine(rc.renderer,
                           top_left.round_x(),
                           top_left.round_y(),
                           top_left.round_x(),
                           bottom_right.round_y());
        SDL_RenderDrawLine(rc.renderer,
                           bottom_right.round_x(),
                           top_left.round_y(),
                           bottom_right.round_x(),
                           bottom_right.round_y());
      }

      return Lisple::Constant::NIL;
    }

    EXEC_BODY(DrawRectBang, exec_draw_rect_from_points)
    {
      const Point& top_left = Lisple::obj<Point>(*args[0]);
      const Point& bottom_right = Lisple::obj<Point>(*args[1]);

      Lisple::sptr_rtval_v n_args{
        RectAdapter::make_unique(Rect{static_cast<int>(top_left.x),
                                      static_cast<int>(top_left.y),
                                      static_cast<int>(bottom_right.x - top_left.x),
                                      static_cast<int>(bottom_right.y - top_left.y)}),
        args[2]};

      return exec_draw_rect(ctx, n_args);
    }

    /* RenderTextBang - text! */
    FUNC_IMPL(
      RenderTextBang,
      MULTI_SIG((FN_ARGS((&Lisple::Type::STRING), (&HostType::POINT)),
                 EXEC_DISPATCH(&RenderTextBang::exec_text_no_opts)),
                (FN_ARGS((&Lisple::Type::STRING), (&HostType::POINT), (&Lisple::Type::MAP)),
                 EXEC_DISPATCH(&RenderTextBang::exec_text))));

    static Lisple::MapSchema text_opts_schema({},
                                              {{"font", &Lisple::Type::KEY},
                                               {"color", &HostType::COLOR},
                                               {"scale", &Lisple::Type::NUMBER},
                                               {"font-styles", &Lisple::Type::ANY},
                                               {"shadow", &Lisple::Type::ANY},
                                               {"marked-style", &Lisple::Type::ANY}});

    static std::vector<Text::FontStyle> parse_font_styles(const Lisple::sptr_rtval& value)
    {
      if (!value || value->type == Lisple::RTValue::Type::NIL) return {};

      auto parse_one = [](const Lisple::sptr_rtval& style_value)
      {
        if (!style_value || style_value->type != Lisple::RTValue::Type::KEYWORD)
        {
          throw Lisple::TypeError("Text font style must be a keyword");
        }

        if (style_value->str() == "underline") return Text::FontStyle::UNDERLINE;
        throw Lisple::TypeError("Unknown text font style: " + style_value->to_string());
      };

      if (value->type == Lisple::RTValue::Type::KEYWORD) return {parse_one(value)};
      if (value->type != Lisple::RTValue::Type::VECTOR)
      {
        throw Lisple::TypeError("Text font styles must be a keyword or vector");
      }

      std::vector<Text::FontStyle> out;
      for (auto& child : Lisple::get_children(*value))
      {
        out.push_back(parse_one(child));
      }
      return out;
    }

    static std::vector<Text::Shadow> parse_shadows(Lisple::Context& ctx,
                                                   const Lisple::sptr_rtval& shadow_val)
    {
      std::vector<Text::Shadow> shadows;
      if (!shadow_val || shadow_val->type == Lisple::RTValue::Type::NIL) return shadows;

      static Lisple::MapSchema shadow_schema(
        {{"offset", &HostType::POINT}, {"color", &HostType::COLOR}},
        {});

      auto parse_one = [&](const Lisple::sptr_rtval& s)
      {
        auto sh = shadow_schema.bind(ctx, *s);
        return Text::Shadow(sh.obj<Point>("offset"), sh.obj<Color>("color"));
      };

      if (shadow_val->type == Lisple::RTValue::Type::VECTOR)
      {
        for (auto& s : Lisple::get_children(*shadow_val))
          shadows.push_back(parse_one(s));
      }
      else if (shadow_val->type == Lisple::RTValue::Type::MAP)
      {
        shadows.push_back(parse_one(shadow_val));
      }

      return shadows;
    }

    static std::optional<char> parse_inline_marker(const Lisple::sptr_rtval& value)
    {
      if (!value || value->type == Lisple::RTValue::Type::NIL) return std::nullopt;
      if (value->type == Lisple::RTValue::Type::CHAR) return static_cast<char>(value->ch());
      if (value->type == Lisple::RTValue::Type::STRING ||
          value->type == Lisple::RTValue::Type::KEYWORD ||
          value->type == Lisple::RTValue::Type::SYMBOL)
      {
        std::string raw = value->str();
        if (raw.size() == 1) return raw[0];
      }
      return std::nullopt;
    }

    static std::optional<Text::InlineTextStyleSpec> parse_marked_style(
      Lisple::Context& ctx,
      const Lisple::sptr_rtval& value)
    {
      if (!value || value->type == Lisple::RTValue::Type::NIL) return std::nullopt;

      static Lisple::MapSchema inline_schema({},
                                             {{"enabled", &Lisple::Type::BOOL},
                                              {"marker", &Lisple::Type::ANY},
                                              {"font", &Lisple::Type::KEY},
                                              {"color", &HostType::COLOR},
                                              {"scale", &Lisple::Type::NUMBER},
                                              {"font-styles", &Lisple::Type::ANY},
                                              {"shadow", &Lisple::Type::ANY}});

      auto inline_source = value;
      if (Lisple::Dict::contains_key(*value, "color"))
      {
        auto color_value = Lisple::Dict::get_property(*value, "color");
        if (color_value && color_value->type == Lisple::RTValue::Type::KEYWORD &&
            color_value->str() == "none")
        {
          inline_source = Lisple::Dict::shallow_copy(value);
          Lisple::Dict::set_property(inline_source,
                                     Lisple::RTValue::keyword("color"),
                                     Lisple::Constant::NIL);
        }
      }

      auto opts = inline_schema.bind(ctx, *inline_source);
      Text::InlineTextStyleSpec spec;
      spec.enabled = opts.contains("enabled") ? opts.boolean("enabled") : true;
      if (auto marker = parse_inline_marker(opts.val("marker")); marker.has_value())
      {
        spec.marker = *marker;
      }
      if (opts.contains("font")) spec.font_key = opts.str("font");
      if (auto color_value = opts.val("color");
          color_value && color_value->type == Lisple::RTValue::Type::KEYWORD &&
          color_value->str() == "none")
      {
        spec.use_font_color = true;
      }
      else if (auto color_value = opts.val("color");
               color_value && color_value->type != Lisple::RTValue::Type::NIL)
      {
        spec.color = Lisple::obj<Color>(*color_value);
      }
      if (opts.contains("scale")) spec.scale = opts.i32("scale");
      if (opts.contains("font-styles"))
      {
        spec.font_styles = parse_font_styles(opts.val("font-styles"));
      }
      if (opts.contains("shadow"))
      {
        spec.shadows = parse_shadows(ctx, opts.val("shadow"));
      }
      return spec;
    }

    static Lisple::sptr_rtval make_rect_map(int x, int y, int w, int h)
    {
      auto map = Lisple::RTValue::map({});
      auto vx = Lisple::RTValue::number(x);
      auto vy = Lisple::RTValue::number(y);
      auto vw = Lisple::RTValue::number(w);
      auto vh = Lisple::RTValue::number(h);
      Lisple::Dict::set_property(map, Lisple::RTValue::keyword("x"), vx);
      Lisple::Dict::set_property(map, Lisple::RTValue::keyword("y"), vy);
      Lisple::Dict::set_property(map, Lisple::RTValue::keyword("w"), vw);
      Lisple::Dict::set_property(map, Lisple::RTValue::keyword("h"), vh);
      return map;
    }

    EXEC_BODY(RenderTextBang, exec_text_no_opts)
    {
      Lisple::sptr_rtval_v full_args = args;
      full_args.push_back(Lisple::RTValue::map({}));
      return this->exec_text(ctx, full_args);
    }

    EXEC_BODY(RenderTextBang, exec_text)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const std::string& text = args[0]->str();
      const Point& pos = Lisple::obj<Point>(*args[1]);
      auto opts = text_opts_schema.bind(ctx, *args[2]);

      std::string font_key = "font/console";
      if (auto fv = opts.val("font"); fv && fv->type == Lisple::RTValue::Type::KEYWORD)
        font_key = fv->str();

      int scale = 1;
      if (auto sv = opts.val("scale"); sv && sv->type != Lisple::RTValue::Type::NIL)
        scale = sv->num().get_int();

      std::optional<Color> color;
      if (auto cv = opts.val("color"); cv && cv->type != Lisple::RTValue::Type::NIL)
      {
        color = Lisple::obj<Color>(*cv);
      }

      std::vector<Text::FontStyle> font_styles;
      if (auto fsv = opts.val("font-styles"); fsv && fsv->type != Lisple::RTValue::Type::NIL)
      {
        font_styles = parse_font_styles(fsv);
      }

      std::vector<Text::Shadow> shadows;
      if (auto sv = opts.val("shadow"); sv && sv->type != Lisple::RTValue::Type::NIL)
      {
        shadows = parse_shadows(ctx, sv);
      }

      auto inline_style = parse_marked_style(ctx, opts.val("marked-style"));

      auto text_op = Text::make_text_render_op(rc,
                                               font_key,
                                               scale,
                                               color,
                                               font_styles,
                                               shadows,
                                               inline_style);
      if (!text_op) return Lisple::Constant::NIL;

      Text::render_text(rc, *text_op, text, pos.round_x(), pos.round_y());

      SDL_Rect size = Text::calculate_rendered_size(rc, *text_op, text);
      return make_rect_map(pos.round_x(), pos.round_y(), size.w, size.h);
    }

    /* TextSize - text-size */
    FUNC_IMPL(TextSize,
              MULTI_SIG((FN_ARGS((&Lisple::Type::STRING)),
                         EXEC_DISPATCH(&TextSize::exec_size_no_opts)),
                        (FN_ARGS((&Lisple::Type::STRING), (&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&TextSize::exec_size))));

    static Lisple::MapSchema text_size_opts_schema({},
                                                   {{"font", &Lisple::Type::KEY},
                                                    {"scale", &Lisple::Type::NUMBER},
                                                    {"font-styles", &Lisple::Type::ANY},
                                                    {"shadow", &Lisple::Type::ANY},
                                                    {"marked-style", &Lisple::Type::ANY}});

    EXEC_BODY(TextSize, exec_size_no_opts)
    {
      Lisple::sptr_rtval_v full_args = args;
      full_args.push_back(Lisple::RTValue::map({}));
      return this->exec_size(ctx, full_args);
    }

    EXEC_BODY(TextSize, exec_size)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const std::string& text = args[0]->str();
      auto opts = text_size_opts_schema.bind(ctx, *args[1]);

      std::string font_key = "font/console";
      if (auto fv = opts.val("font"); fv && fv->type == Lisple::RTValue::Type::KEYWORD)
        font_key = fv->str();

      int scale = 1;
      if (auto sv = opts.val("scale"); sv && sv->type != Lisple::RTValue::Type::NIL)
        scale = sv->num().get_int();

      std::vector<Text::FontStyle> font_styles;
      if (auto fsv = opts.val("font-styles"); fsv && fsv->type != Lisple::RTValue::Type::NIL)
      {
        font_styles = parse_font_styles(fsv);
      }

      std::vector<Text::Shadow> shadows;
      if (auto sv = opts.val("shadow"); sv && sv->type != Lisple::RTValue::Type::NIL)
      {
        shadows = parse_shadows(ctx, sv);
      }

      auto inline_style = parse_marked_style(ctx, opts.val("marked-style"));

      auto text_op = Text::make_text_render_op(rc,
                                               font_key,
                                               scale,
                                               std::nullopt,
                                               font_styles,
                                               shadows,
                                               inline_style);
      if (!text_op) return Lisple::Constant::NIL;

      SDL_Rect size = Text::calculate_rendered_size(rc, *text_op, text);

      auto map = Lisple::RTValue::map({});
      auto vw = Lisple::RTValue::number(size.w);
      auto vh = Lisple::RTValue::number(size.h);
      Lisple::Dict::set_property(map, Lisple::RTValue::keyword("w"), vw);
      Lisple::Dict::set_property(map, Lisple::RTValue::keyword("h"), vh);
      return map;
    }

    /* UseColorBang */
    FUNC_IMPL(UseColorBang,
              MULTI_SIG((FN_ARGS((&HostType::COLOR)),
                         EXEC_DISPATCH(&UseColorBang::exec_use_color)),
                        (FN_ARGS((&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER),
                                 (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&UseColorBang::exec_use_color_num))));

    EXEC_BODY(UseColorBang, exec_use_color)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const Color& color = Lisple::obj<Color>(*args[0]);
      SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);

      return Lisple::Constant::NIL;
    }

    EXEC_BODY(UseColorBang, exec_use_color_num)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      const int r = args[0]->num().get_int();
      const int g = args[1]->num().get_int();
      const int b = args[2]->num().get_int();
      const int a = args[3]->num().get_int();

      SDL_SetRenderDrawColor(rc.renderer, r, g, b, a);

      return Lisple::Constant::NIL;
    }

  } // namespace Function

  RenderNamespace::RenderNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__RENDER))
  {
    values.emplace(FN__DRAW_IMAGE_BANG, Function::DrawImageBang::make());
    values.emplace(FN__DRAW_LINE_BANG, Function::DrawLineBang::make());
    values.emplace(FN__DRAW_POLYGON_BANG, Function::DrawPolygonBang::make());
    values.emplace(FN__DRAW_RECT_BANG, Function::DrawRectBang::make());
    values.emplace(FN__RENDER_TEXT_BANG, Function::RenderTextBang::make());
    values.emplace(FN__TEXT_SIZE, Function::TextSize::make());
    values.emplace(FN__USE_COLOR_BANG, Function::UseColorBang::make());
  }
} // namespace Pixils::Script
