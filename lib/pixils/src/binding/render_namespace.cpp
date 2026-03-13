
#include "pixils/binding/arg_collector.h"
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
      SHKEY(OFFSET, "offset");
      SHKEY(ROTATION, "rotation");
    } // namespace MapKey

    namespace Function
    {
      /* DrawLineBang - draw-line! */
      FUNC_IMPL(DrawLineBang, SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                                   EXEC_DISPATCH(&DrawLineBang::draw_line))));

      FUNC_BODY(DrawLineBang, draw_line)
      {
        RenderContext& rc =
            ctx.lookup(ID__PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        const Point& from = args.front()->as<PointAdapter>().get_object();
        const Point& to = args.back()->as<PointAdapter>().get_object();

        SDL_RenderDrawLine(rc.renderer, from.x, from.y, to.x, to.y);

        return Lisple::NIL;
      }

      /* DrawPolygonbang - draw-polygon! */
      FUNC_IMPL(DrawPolygonBang,
                MULTI_SIG((FN_ARGS((&HostType::VECTOR_OF_POINT)),
                           EXEC_DISPATCH(&DrawPolygonBang::draw_polygon)),
                          (FN_ARGS((&HostType::VECTOR_OF_POINT), (&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&DrawPolygonBang::draw_polygon_with_opts))));

      ArgCollector draw_poly_collector(FN__DRAW_POLYGON_BANG, {},
                                       {{*MapKey::CLOSE, &Lisple::Type::BOOL},
                                        {*MapKey::ROTATION, &Lisple::Type::NUMBER},
                                        {*MapKey::OFFSET, &HostType::POINT}});

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
        Point offset = keys.count(MapKey::OFFSET->value)
                           ? ArgCollector::coerce_host_object<Point>(
                                 ctx, keys, *MapKey::OFFSET, &HostType::POINT)
                           : POINT__ZERO_ZERO;

        std::vector<Point> pts;
        if (polygon->size() > 0)
        {
          pts.reserve(polygon->size());
          for (auto& poly_pt : polygon->get_children())
          {
            pts.push_back(poly_pt->as<PointAdapter>()
                              .get_object()
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
            SDL_RenderDrawLine(rc.renderer, from.x, from.y, to.x, to.y);
          }
        }

        return Lisple::NIL;
      }

      /* UseColorBang */
      FUNC_IMPL(UseColorBang, SIG((FN_ARGS((&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER),
                                           (&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER)),
                                   EXEC_DISPATCH(&UseColorBang::use_color))));

      FUNC_BODY(UseColorBang, use_color)
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
      objects.emplace(FN__USE_COLOR_BANG, std::make_shared<Function::UseColorBang>());
    }
  } // namespace Script
} // namespace Pixils
