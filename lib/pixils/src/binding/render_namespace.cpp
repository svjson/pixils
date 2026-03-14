
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/render_namespace.h>
#include <pixils/context.h>

#include <SDL2/SDL_render.h>
#include <lisple/namespace.h>

namespace Pixils
{
  namespace Script
  {
    namespace MapKey
    {
      SHKEY(CLOSE, "close");
      SHKEY(COLOR, "color");
      SHKEY(FILL, "fill");
      SHKEY(OFFSET, "offset");
      SHKEY(ROTATION, "rotation");
      SHKEY(SCALE, "scale");
    } // namespace MapKey

    namespace Function
    {
      /* DrawLineBang - line! */
      FUNC_IMPL(DrawLineBang,
                MULTI_SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                           EXEC_DISPATCH(&DrawLineBang::draw_line)),
                          (FN_ARGS((&HostType::POINT), (&HostType::POINT), (&HostType::COLOR)),
                           EXEC_DISPATCH(&DrawLineBang::draw_line))));

      FUNC_BODY(DrawLineBang, draw_line)
      {
        RenderContext& rc =
            ctx.lookup(ID__PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        const Point& from = args[0]->as<PointAdapter>().get_object();
        const Point& to = args[1]->as<PointAdapter>().get_object();

        if (args.size() == 3 && args[2]->is_truthy())
        {
          const Color& color = args[2]->as<ColorAdapter>().get_object();
          SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
        }

        SDL_RenderDrawLine(rc.renderer, from.round_x(), from.round_y(), to.round_x(),
                           to.round_y());

        return Lisple::NIL;
      }

      /* DrawPolygonbang - polygon! */
      FUNC_IMPL(DrawPolygonBang,
                MULTI_SIG((FN_ARGS((&HostType::VECTOR_OF_POINT)),
                           EXEC_DISPATCH(&DrawPolygonBang::draw_polygon)),
                          (FN_ARGS((&HostType::VECTOR_OF_POINT), (&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&DrawPolygonBang::draw_polygon_with_opts))));

      ArgCollector draw_poly_collector(FN__DRAW_POLYGON_BANG, {},
                                       {{*MapKey::CLOSE, &Lisple::Type::BOOL},
                                        {*MapKey::ROTATION, &Lisple::Type::NUMBER},
                                        {*MapKey::OFFSET, &HostType::POINT},
                                        {*MapKey::COLOR, &HostType::COLOR},
                                        {*MapKey::SCALE, &Lisple::Type::NUMBER}});

      FUNC_BODY(DrawPolygonBang, draw_polygon)
      {
        Lisple::sptr_sobject_v opt_args = args;
        opt_args.push_back(Lisple::Map::make({}));
        return this->draw_polygon_with_opts(ctx, opt_args);
      }

      FUNC_BODY(DrawPolygonBang, draw_polygon_with_opts)
      {
        RenderContext& rc =
            ctx.lookup(ID__PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        str_key_map_t keys = draw_poly_collector.collect_keys(ctx, *args.back());

        Lisple::sptr_sobject& polygon = args.front();
        bool close_shape =
            ArgCollector::bool_value(keys, *MapKey::CLOSE, false) || polygon->size() == 1;
        float rotation = ArgCollector::float_value(keys, *MapKey::ROTATION, 0.0);
        float scale = ArgCollector::float_value(keys, *MapKey::SCALE, 1.0);
        std::optional<Color> color =
            ArgCollector::optional_host_object<Color>(keys, *MapKey::COLOR);
        Point offset = keys.count(MapKey::OFFSET->value)
                           ? ArgCollector::coerce_host_object<Point>(
                                 ctx, keys, *MapKey::OFFSET, &HostType::POINT)
                           : POINT__ZERO_ZERO;

        if (color)
        {
          SDL_SetRenderDrawColor(rc.renderer, color->r, color->g, color->b, color->a);
        }

        std::vector<Point> pts;
        if (polygon->size() > 0)
        {
          pts.reserve(polygon->size());
          for (auto& poly_pt : polygon->get_children())
          {
            pts.push_back((poly_pt->as<PointAdapter>().get_object() * scale)
                              .rotate(POINT__ZERO_ZERO, rotation)
                              .plus(offset.x, offset.y));
          }

          if (close_shape)
          {
            pts.push_back(polygon->get_children()
                              .front()
                              ->as<PointAdapter>()
                              .get_object()
                              .rotate(POINT__ZERO_ZERO, rotation)
                              .plus(offset.x, offset.y));
          }

          for (size_t i = 0; i < pts.size() - 1; i++)
          {
            const Point& from = pts[i];
            const Point& to = pts[i + 1];
            SDL_RenderDrawLine(rc.renderer, from.round_x(), from.round_y(), to.round_x(),
                               to.round_y());
          }
        }

        return Lisple::NIL;
      }

      /* DrawRectBang - rect! */
      FUNC_IMPL(DrawRectBang,
                SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT), (&Lisple::Type::MAP)),
                     EXEC_DISPATCH(&DrawRectBang::draw_rect_from_points))));

      FUNC_BODY(DrawRectBang, draw_rect_from_points)
      {
        RenderContext& rc =
            ctx.lookup(ID__PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        const Point& top_left = args[0]->as<PointAdapter>().get_object();
        const Point& bottom_right = args[1]->as<PointAdapter>().get_object();
        const Lisple::Map& opts = args[2]->as<Lisple::Map>();
        if (opts.get_sptr_property(*MapKey::COLOR)->is_truthy())
        {
          const Color& color =
              opts.get_sptr_property(*MapKey::COLOR)->as<ColorAdapter>().get_object();
          SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
        }

        const Point wh = bottom_right - top_left;

        SDL_Rect rect = {top_left.round_x(), top_left.round_y(), wh.round_x(), wh.round_y()};

        if (opts.get_sptr_property(*MapKey::FILL)->is_truthy())
        {
          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_BLEND);
          SDL_RenderFillRect(rc.renderer, &rect);
          SDL_SetRenderDrawBlendMode(rc.renderer, SDL_BLENDMODE_NONE);
        }
        else
        {
          SDL_RenderDrawLine(rc.renderer, top_left.round_x(), top_left.round_y(),
                             bottom_right.round_x(), top_left.round_y());
          SDL_RenderDrawLine(rc.renderer, top_left.round_x(), bottom_right.round_y(),
                             bottom_right.round_x(), bottom_right.round_y());
          SDL_RenderDrawLine(rc.renderer, top_left.round_x(), top_left.round_y(),
                             top_left.round_x(), bottom_right.round_y());
          SDL_RenderDrawLine(rc.renderer, bottom_right.round_x(), top_left.round_y(),
                             bottom_right.round_x(), bottom_right.round_y());
        }

        return Lisple::NIL;
      }

      /* UseColorBang */
      FUNC_IMPL(UseColorBang,
                MULTI_SIG((FN_ARGS((&HostType::COLOR)),
                           EXEC_DISPATCH(&UseColorBang::use_color)),
                          (FN_ARGS((&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER),
                                   (&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER)),
                           EXEC_DISPATCH(&UseColorBang::use_color_num))));

      FUNC_BODY(UseColorBang, use_color)
      {
        RenderContext& rc =
            ctx.lookup(ID__PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        const Color& color = args.front()->as<ColorAdapter>().get_object();
        SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);

        return Lisple::NIL;
      }

      FUNC_BODY(UseColorBang, use_color_num)
      {
        RenderContext& rc =
            ctx.lookup(ID__PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        const int r = args[0]->as<Lisple::Number>().int_value();
        const int g = args[1]->as<Lisple::Number>().int_value();
        const int b = args[2]->as<Lisple::Number>().int_value();
        const int a = args[3]->as<Lisple::Number>().int_value();

        SDL_SetRenderDrawColor(rc.renderer, r, g, b, a);

        return Lisple::NIL;
      }

    } // namespace Function

    RenderNamespace::RenderNamespace()
        : Lisple::Namespace(NS__PIXILS__RENDER)
    {
      objects.emplace(FN__DRAW_LINE_BANG, std::make_shared<Function::DrawLineBang>());
      objects.emplace(FN__DRAW_POLYGON_BANG, std::make_shared<Function::DrawPolygonBang>());
      objects.emplace(FN__DRAW_RECT_BANG, std::make_shared<Function::DrawRectBang>());
      objects.emplace(FN__USE_COLOR_BANG, std::make_shared<Function::UseColorBang>());
    }
  } // namespace Script
} // namespace Pixils
