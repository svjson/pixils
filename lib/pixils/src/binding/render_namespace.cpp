
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

        bool close_shape = args.back()->get_property(Lisple::Key("close")).is_truthy();

        for (size_t i = 0; i < args.front()->size() - (close_shape ? 0 : 1); i++)
        {
          const Point& from = args.front()->get_children()[i]->as<PointAdapter>().get_object();
          const Point& to = args.front()
                                ->get_children()[i + 1 == args.front()->size() ? 0 : i + 1]
                                ->as<PointAdapter>()
                                .get_object();
          SDL_RenderDrawLine(rc.renderer, from.x, from.y, to.x, to.y);
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
