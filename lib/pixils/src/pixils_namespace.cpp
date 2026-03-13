
#include <pixils/arg_collector.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>
#include <pixils/pixils_namespace.h>

#include <SDL2/SDL_render.h>
#include <lisple/exec.h>
#include <lisple/host.h>

namespace Pixils
{
  namespace Script
  {
    const Lisple::Word PIXILS__RENDER_CONTEXT("pixils/render-context");

    namespace MapKey
    {
      SHKEY(BUFFER_SIZE, "buffer-size");
      SHKEY(H, "h");
      SHKEY(HELD_KEYS, "held-keys");
      SHKEY(KEY_DOWN, "key-down");
      SHKEY(PIXEL_SIZE, "pixel-size");
      SHKEY(W, "w");
      SHKEY(X, "x");
      SHKEY(Y, "y");
    } // namespace MapKey

    namespace Function
    {
      /* DrawLineBang - draw-ling! */
      FUNC_IMPL(DrawLineBang, SIG((FN_ARGS((&HostType::COORDINATE), (&HostType::COORDINATE)),
                                   EXEC_DISPATCH(&DrawLineBang::draw_line))));

      FUNC_BODY(DrawLineBang, draw_line)
      {
        RenderContext& rc =
            ctx.lookup(PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        const Coordinate& from = args.front()->as<CoordinateAdapter>().get_object();
        const Coordinate& to = args.back()->as<CoordinateAdapter>().get_object();

        SDL_RenderDrawLine(rc.renderer, from.x, from.y, to.x, to.y);

        return Lisple::NIL;
      }

      /* DrawPolygonbang - draw-polygon! */
      FUNC_IMPL(DrawPolygonBang,
                MULTI_SIG((FN_ARGS((&HostType::VECTOR_OF_COORDINATE)),
                           EXEC_DISPATCH(&DrawPolygonBang::draw_polygon)),
                          (FN_ARGS((&HostType::VECTOR_OF_COORDINATE), (&Lisple::Type::MAP)),
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
            ctx.lookup(PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        bool close_shape = args.back()->get_property(Lisple::Key("close")).is_truthy();

        for (size_t i = 0; i < args.front()->size() - (close_shape ? 0 : 1); i++)
        {
          const Coordinate& from =
              args.front()->get_children()[i]->as<CoordinateAdapter>().get_object();
          const Coordinate& to = args.front()
                                     ->get_children()[i + 1 == args.front()->size() ? 0 : i + 1]
                                     ->as<CoordinateAdapter>()
                                     .get_object();
          SDL_RenderDrawLine(rc.renderer, from.x, from.y, to.x, to.y);
        }

        return Lisple::NIL;
      }

      /* Distance Between - distance-between */
      FUNC_IMPL(DistanceBetween, SIG((FN_ARGS((&HostType::COORDINATE), (&HostType::COORDINATE)),
                                      EXEC_DISPATCH(&DistanceBetween::distance_between_points))))

      FUNC_BODY(DistanceBetween, distance_between_points)
      {
        const Coordinate& a = args[0]->as<CoordinateAdapter>().get_object();
        const Coordinate& b = args[1]->as<CoordinateAdapter>().get_object();

        return Lisple::Number::make(a.distance_to(b));
      }

      /* Rotate Coordinate - rotate-coordinate */
      FUNC_IMPL(RotateCoordinate, SIG((FN_ARGS((&HostType::COORDINATE), (&HostType::COORDINATE),
                                               (&Lisple::Type::NUMBER)),
                                       EXEC_DISPATCH(&RotateCoordinate::rotate_coordinate))))

      FUNC_BODY(RotateCoordinate, rotate_coordinate)
      {
        const Coordinate& point = args[0]->as<CoordinateAdapter>().get_object();
        const Coordinate& origin = args[1]->as<CoordinateAdapter>().get_object();
        float radians = args[2]->as<Lisple::Number>().float_value();

        float s = std::sin(radians);
        float c = std::cos(radians);

        // translate point to origin
        float x = point.x - origin.x;
        float y = point.y - origin.y;

        // rotate
        float x_new = x * c - y * s;
        float y_new = x * s + y * c;

        // translate back
        x_new += origin.x;
        y_new += origin.y;

        return CoordinateAdapter::make<Coordinate>(x_new, y_new);
      }

      /* CoordinateDivision */
      FUNC_IMPL(CoordinateDivision, SIG((FN_ARGS((&HostType::COORDINATE), (&Lisple::Type::NUMBER)),
                                         EXEC_DISPATCH(&CoordinateDivision::divide_num))));

      FUNC_BODY(CoordinateDivision, divide_num)
      {
        const Coordinate& coord = args.front()->as<CoordinateAdapter>().get_object();
        const float n = args.back()->as<Lisple::Number>().float_value();

        return CoordinateAdapter::make<Coordinate>(coord.x / n, coord.y / n);
      }

      /* CoordinatePlus */
      FUNC_IMPL(CoordinatePlus, SIG((FN_ARGS((&HostType::COORDINATE), (&HostType::COORDINATE)),
                                     EXEC_DISPATCH(&CoordinatePlus::plus))));

      FUNC_BODY(CoordinatePlus, plus)
      {
        return CoordinateAdapter::make<Coordinate>(
            args.front()->as<CoordinateAdapter>().get_object() +
            args.back()->as<CoordinateAdapter>().get_object());
      }

      /* CoordinateMinus */
      FUNC_IMPL(CoordinateMinus, SIG((FN_ARGS((&HostType::COORDINATE), (&HostType::COORDINATE)),
                                      EXEC_DISPATCH(&CoordinateMinus::minus))));

      FUNC_BODY(CoordinateMinus, minus)
      {
        return CoordinateAdapter::make<Coordinate>(
            args.front()->as<CoordinateAdapter>().get_object() -
            args.back()->as<CoordinateAdapter>().get_object());
      }

      /* UseColorBang */
      FUNC_IMPL(UseColorBang, SIG((FN_ARGS((&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER),
                                           (&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER)),
                                   EXEC_DISPATCH(&UseColorBang::use_color))));

      FUNC_BODY(UseColorBang, use_color)
      {
        RenderContext& rc =
            ctx.lookup(PIXILS__RENDER_CONTEXT)->as<RenderContextAdapter>().get_object();

        const int r = args[0]->as<Lisple::Number>().int_value();
        const int g = args[1]->as<Lisple::Number>().int_value();
        const int b = args[2]->as<Lisple::Number>().int_value();
        const int a = args[3]->as<Lisple::Number>().int_value();

        SDL_SetRenderDrawColor(rc.renderer, r, g, b, a);

        return Lisple::NIL;
      }

      /* Coordinate make-function */
      FUNC_IMPL(MakeCoordinate,
                MULTI_SIG((FN_ARGS((&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER)),
                           EXEC_DISPATCH(&MakeCoordinate::make_coordinate_from_ints)),
                          (FN_ARGS((&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&MakeCoordinate::make_coordinate_from_map))));

      ArgCollector coordinate_collector(FN__MAKE_COORDINATE, {{*MapKey::X, &Lisple::Type::NUMBER},
                                                              {*MapKey::Y, &Lisple::Type::NUMBER}});

      Coordinate unbox_coordinate(Lisple::Context& ctx, Lisple::sptr_sobject& obj)
      {
        if (HostType::COORDINATE.is_type_of(*obj))
        {
          return obj->as<CoordinateAdapter>().get_object();
        }
        else if (Lisple::Type::MAP.is_type_of(*obj))
        {
          return ctx.call(NS_PIXILS + "/" + FN__MAKE_COORDINATE, obj)
              ->as<CoordinateAdapter>()
              .get_object();
        }
        else
        {
          throw Lisple::TypeError("Cannot be interpereted as Coordinate: " + obj->to_string());
        }
      }

      FUNC_BODY(MakeCoordinate, make_coordinate_from_map)
      {
        if (*args[0] == *Lisple::NIL)
        {
          throw Lisple::TypeError("Cannot create Coordinate from nil");
        }
        str_key_map_t keys = coordinate_collector.collect_keys(*args.front());
        return CoordinateAdapter::make<Coordinate>(ArgCollector::float_value(keys, *MapKey::X),
                                                   ArgCollector::float_value(keys, *MapKey::Y));
      }

      FUNC_BODY(MakeCoordinate, make_coordinate_from_ints)
      {
        return CoordinateAdapter::make<Coordinate>(Lisple::int_val(*args.at(0)),
                                                   Lisple::int_val(*args.at(1)));
      }

      /* Dimension make function */
      FUNC_IMPL(MakeDimension,
                SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeDimension::make))))

      ArgCollector dimension_collector(FN__MAKE_DIMENSION, {{*MapKey::W, &Lisple::Type::NUMBER},
                                                            {*MapKey::H, &Lisple::Type::NUMBER}});

      FUNC_BODY(MakeDimension, make)
      {
        str_key_map_t keys = dimension_collector.collect_keys(ctx, *args.front());
        return DimensionAdapter::make<Dimension>(ArgCollector::int_value(keys, *MapKey::W),
                                                 ArgCollector::int_value(keys, *MapKey::H));
      }

    } // namespace Function

    /* CoordinateAdapter */
    HOST_ADAPTER_IMPL(CoordinateAdapter, Coordinate, &HostType::COORDINATE,
                      ({K_GET_SET(CoordinateAdapter, MapKey::X, x),
                        K_GET_SET(CoordinateAdapter, MapKey::Y, y)}));

    ADAPTER_PROP_GET_SET__FIELD(CoordinateAdapter, x, Lisple::Number, x);
    ADAPTER_PROP_GET_SET__FIELD(CoordinateAdapter, y, Lisple::Number, y);

    /* DimensionAdapter */
    HOST_ADAPTER_IMPL(DimensionAdapter, Dimension, &HostType::DIMENSION,
                      ({K_GET_SET(DimensionAdapter, MapKey::W, w),
                        K_GET_SET(DimensionAdapter, MapKey::H, h)}));

    ADAPTER_PROP_GET_SET__FIELD(DimensionAdapter, w, Lisple::Number, w);
    ADAPTER_PROP_GET_SET__FIELD(DimensionAdapter, h, Lisple::Number, h);

    /* FrameEventsAdapter */
    HOST_ADAPTER_IMPL(FrameEventsAdapter, FrameEvents, &HostType::FRAME_EVENTS,
                      ({K_GET(FrameEventsAdapter, MapKey::KEY_DOWN, key_down),
                        K_GET(FrameEventsAdapter, MapKey::HELD_KEYS, held_keys)}))

    ADAPTER_PROP_GET(FrameEventsAdapter, key_down)
    {
      return object->get_object().key_down;
    }

    ADAPTER_PROP_GET(FrameEventsAdapter, held_keys)
    {
      return object->get_object().held_keys;
    }

    /* RenderContextAdapter */
    HOST_ADAPTER_IMPL(RenderContextAdapter, RenderContext, &HostType::RENDER_CONTEXT,
                      ({K_GET(RenderContextAdapter, MapKey::PIXEL_SIZE, pixel_size),
                        K_GET(RenderContextAdapter, MapKey::BUFFER_SIZE, buffer_size)}));

    ADAPTER_PROP_GET__FIELD(RenderContextAdapter, pixel_size, Lisple::Number);
    ADAPTER_PROP_GET__FIELD(RenderContextAdapter, buffer_size, DimensionAdapter, buffer_dim);

    PixilsNamespace::PixilsNamespace(const RenderContext& render_context)
        : Lisple::Namespace(NS_PIXILS)
    {
      objects.emplace("render-context", RenderContextAdapter::make_ref(render_context));
      objects.emplace(FN__MAKE_COORDINATE, std::make_shared<Function::MakeCoordinate>());
      objects.emplace(FN__COORDINATE_DIVISION, std::make_shared<Function::CoordinateDivision>());
      objects.emplace(FN__COORDINATE_MINUS, std::make_shared<Function::CoordinateMinus>());
      objects.emplace(FN__COORDINATE_PLUS, std::make_shared<Function::CoordinatePlus>());
      objects.emplace(FN__DISTANCE_BETWEEN, std::make_shared<Function::DistanceBetween>());
      objects.emplace(FN__DRAW_LINE_BANG, std::make_shared<Function::DrawLineBang>());
      objects.emplace(FN__DRAW_POLYGON_BANG, std::make_shared<Function::DrawPolygonBang>());
      objects.emplace(FN__ROTATE_COORDINATE, std::make_shared<Function::RotateCoordinate>());
      objects.emplace(FN__USE_COLOR_BANG, std::make_shared<Function::UseColorBang>());
    }

  } // namespace Script
} // namespace Pixils
