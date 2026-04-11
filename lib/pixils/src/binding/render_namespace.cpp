
#include "pixils/geom.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/render_namespace.h>
#include <pixils/context.h>

#include <SDL2/SDL_render.h>
#include <lisple/host/schema.h>
#include <lisple/namespace.h>
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
    values.emplace(FN__USE_COLOR_BANG, Function::UseColorBang::make());
  }
} // namespace Pixils::Script
