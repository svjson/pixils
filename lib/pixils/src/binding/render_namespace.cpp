
#include "pixils/geom.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/render_namespace.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>

#include <SDL2/SDL_render.h>
#include <lisple/host/schema.h>
#include <lisple/namespace.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

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
  } // namespace MapKey

  namespace Function
  {
    FUNC_IMPL(DrawImageBang,
              SIG((FN_ARGS((&Lisple::Type::KEY), (&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&DrawImageBang::exec_draw_img))));

    Lisple::MapSchema draw_image_opts_schema({{"pos", &HostType::POINT}},
                                             {{"scale", &Lisple::Type::NUMBER},
                                              {"alpha", &Lisple::Type::NUMBER}});

    EXEC_BODY(DrawImageBang, exec_draw_img)
    {
      auto [asset_bundle, asset_key] = args.front()->qual();
      auto opts = draw_image_opts_schema.bind(ctx, *args[1]);

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

      SDL_Texture* texture = rc.asset_registry->get_image(asset_bundle, asset_key);

      Point& pos = opts.obj<Point>("pos");
      float scale = opts.f32("scale", 1.0f);

      SDL_Rect dim{pos.round_x(), pos.round_y(), 0, 0};
      SDL_QueryTexture(texture, nullptr, nullptr, &dim.w, &dim.h);

      dim.w *= scale;
      dim.h *= scale;

      if (texture)
      {
        SDL_RenderCopy(rc.renderer, texture, nullptr, &dim);
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
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));
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
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

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
    FUNC_IMPL(DrawRectBang,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT), (&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&DrawRectBang::exec_draw_rect_from_points))));

    EXEC_BODY(DrawRectBang, exec_draw_rect_from_points)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

      const Point& top_left = Lisple::obj<Point>(*args[0]);
      const Point& bottom_right = Lisple::obj<Point>(*args[1]);

      static Lisple::MapSchema opts_schema(
        {},
        {{"color", &HostType::COLOR}, {"fill", &Lisple::Type::BOOL}});

      auto opts = opts_schema.bind(ctx, *args[2]);

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
                                               {"shadow", &Lisple::Type::ANY}});

    static void render_shadows(Text::Renderer& renderer,
                               Lisple::Context& ctx,
                               RenderContext& rc,
                               const std::string& text,
                               int x,
                               int y,
                               const Lisple::sptr_rtval& shadow_val)
    {
      static Lisple::MapSchema shadow_schema(
        {{"offset", &HostType::POINT}, {"color", &HostType::COLOR}},
        {});

      auto render_one = [&](const Lisple::sptr_rtval& s)
      {
        auto sh = shadow_schema.bind(ctx, *s);
        const Point& off = sh.obj<Point>("offset");
        const Color& col = sh.obj<Color>("color");
        SDL_Color sdl = col.to_SDL_Color();
        renderer.set_alt_color(sdl);
        renderer.render_text(rc, text, x + off.round_x(), y + off.round_y(), sdl);
      };

      if (shadow_val->type == Lisple::RTValue::Type::VECTOR)
      {
        for (auto& s : Lisple::get_children(*shadow_val))
          render_one(s);
      }
      else if (shadow_val->type == Lisple::RTValue::Type::MAP)
      {
        render_one(shadow_val);
      }
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
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

      const std::string& text = args[0]->str();
      const Point& pos = Lisple::obj<Point>(*args[1]);
      auto opts = text_opts_schema.bind(ctx, *args[2]);

      std::string font_key = "font/console";
      if (auto fv = opts.val("font"); fv && fv->type == Lisple::RTValue::Type::KEYWORD)
        font_key = fv->str();

      BitmapFont* font = rc.font_registry ? rc.font_registry->get_font(font_key) : nullptr;
      if (!font) return Lisple::Constant::NIL;

      Text::Renderer& renderer = font->renderer;

      if (auto sv = opts.val("scale"); sv && sv->type != Lisple::RTValue::Type::NIL)
        renderer.set_scale(sv->num().get_int());
      else
        renderer.set_scale(1);

      SDL_Color color = {0xff, 0xff, 0xff, 0xff};
      if (auto cv = opts.val("color"); cv && cv->type != Lisple::RTValue::Type::NIL)
      {
        const Color& c = Lisple::obj<Color>(*cv);
        color = c.to_SDL_Color();
      }

      if (auto sv = opts.val("shadow"); sv && sv->type != Lisple::RTValue::Type::NIL)
        render_shadows(renderer, ctx, rc, text, pos.round_x(), pos.round_y(), sv);

      renderer.render_text(rc, text, pos.round_x(), pos.round_y(), color);

      SDL_Rect size = renderer.get_rendered_size(rc, text);
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
                                                    {"scale", &Lisple::Type::NUMBER}});

    EXEC_BODY(TextSize, exec_size_no_opts)
    {
      Lisple::sptr_rtval_v full_args = args;
      full_args.push_back(Lisple::RTValue::map({}));
      return this->exec_size(ctx, full_args);
    }

    EXEC_BODY(TextSize, exec_size)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

      const std::string& text = args[0]->str();
      auto opts = text_size_opts_schema.bind(ctx, *args[1]);

      std::string font_key = "font/console";
      if (auto fv = opts.val("font"); fv && fv->type == Lisple::RTValue::Type::KEYWORD)
        font_key = fv->str();

      BitmapFont* font = rc.font_registry ? rc.font_registry->get_font(font_key) : nullptr;
      if (!font) return Lisple::Constant::NIL;

      Text::Renderer& renderer = font->renderer;

      if (auto sv = opts.val("scale"); sv && sv->type != Lisple::RTValue::Type::NIL)
        renderer.set_scale(sv->num().get_int());
      else
        renderer.set_scale(1);

      SDL_Rect size = renderer.get_rendered_size(rc, text);

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
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

      const Color& color = Lisple::obj<Color>(*args[0]);
      SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);

      return Lisple::Constant::NIL;
    }

    EXEC_BODY(UseColorBang, exec_use_color_num)
    {
      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

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
