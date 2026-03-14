
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/geom.h>

#include <lisple/exception.h>

namespace Pixils::Script
{

  namespace MapKey
  {
    SHKEY(ORIGIN, "origin");
    SHKEY(RADIANS, "radians");
    SHKEY(X, "x");
    SHKEY(Y, "y");
  } // namespace MapKey

  namespace Function
  {
    /* Point make-function */
    FUNC_IMPL(MakePoint, MULTI_SIG((FN_ARGS((&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER)),
                                    EXEC_DISPATCH(&MakePoint::make_point_from_ints)),
                                   (FN_ARGS((&Lisple::Type::MAP)),
                                    EXEC_DISPATCH(&MakePoint::make_point_from_map))));

    ArgCollector point_collector(std::string(FN__MAKE_POINT),
                                 {{*MapKey::X, &Lisple::Type::NUMBER},
                                  {*MapKey::Y, &Lisple::Type::NUMBER}});

    Point unbox_point(Lisple::Context& ctx, Lisple::sptr_sobject& obj)
    {
      if (HostType::POINT.is_type_of(*obj))
      {
        return obj->as<PointAdapter>().get_object();
      }
      else if (Lisple::Type::MAP.is_type_of(*obj))
      {
        return ctx.call(FN__PIXILS__POINT__MAKE_POINT, obj)->as<PointAdapter>().get_object();
      }
      else
      {
        throw Lisple::TypeError("Cannot be interpereted as Point: " + obj->to_string());
      }
    }

    FUNC_BODY(MakePoint, make_point_from_map)
    {
      if (*args[0] == *Lisple::NIL)
      {
        throw Lisple::TypeError("Cannot create Point from nil");
      }
      str_key_map_t keys = point_collector.collect_keys(*args.front());
      return PointAdapter::make<Point>(ArgCollector::float_value(keys, *MapKey::X),
                                       ArgCollector::float_value(keys, *MapKey::Y));
    }

    FUNC_BODY(MakePoint, make_point_from_ints)
    {
      return PointAdapter::make<Point>(Lisple::int_val(*args.at(0)),
                                       Lisple::int_val(*args.at(1)));
    }

    /* Distance Between - distance-between */
    FUNC_IMPL(DistanceBetween, SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                                    EXEC_DISPATCH(&DistanceBetween::distance_between_points))))

    FUNC_BODY(DistanceBetween, distance_between_points)
    {
      const Point& a = args[0]->as<PointAdapter>().get_object();
      const Point& b = args[1]->as<PointAdapter>().get_object();

      return Lisple::Number::make(a.distance_to(b));
    }

    /* Rotate Point - rotate-point */
    FUNC_IMPL(RotatePoint, MULTI_SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT),
                                              (&Lisple::Type::NUMBER)),
                                      EXEC_DISPATCH(&RotatePoint::rotate_point_orig_amount)),
                                     (FN_ARGS((&HostType::POINT), (&Lisple::Type::NUMBER)),
                                      EXEC_DISPATCH(&RotatePoint::rotate_point_amount)),
                                     (FN_ARGS((&HostType::POINT), (&Lisple::Type::MAP)),
                                      EXEC_DISPATCH(&RotatePoint::rotate_point_with_opts))))

    ArgCollector rotate_opts_collector(std::string(FN__ROTATE), {},
                                       {{*MapKey::ORIGIN, &HostType::POINT},
                                        {*MapKey::RADIANS, &Lisple::Type::NUMBER}});

    FUNC_BODY(RotatePoint, rotate_point_with_opts)
    {
      const Point& point = args[0]->as<PointAdapter>().get_object();
      str_key_map_t keys = rotate_opts_collector.collect_keys(ctx, *args[1]);

      return PointAdapter::make<Point>(point.rotate(
          keys.count(MapKey::ORIGIN->value) ? ArgCollector::coerce_host_object<Point>(
                                                  ctx, keys, *MapKey::ORIGIN, &HostType::POINT)
                                            : POINT__ZERO_ZERO,
          ArgCollector::float_value(keys, *MapKey::RADIANS, 0.0)));
    }

    FUNC_BODY(RotatePoint, rotate_point_amount)
    {
      Lisple::sptr_sobject_v fwd_args = args;
      fwd_args[1] = Lisple::Map::make({MapKey::RADIANS, args[1]});
      return this->rotate_point_with_opts(ctx, fwd_args);
    }

    FUNC_BODY(RotatePoint, rotate_point_orig_amount)
    {
      Lisple::sptr_sobject_v fwd_args = args;
      fwd_args[1] = Lisple::Map::make({MapKey::ORIGIN, args[1], MapKey::RADIANS, args[2]});
      return this->rotate_point_with_opts(ctx, fwd_args);
    }

    /* PointDivision */
    FUNC_IMPL(PointDivision, SIG((FN_ARGS((&HostType::POINT), (&Lisple::Type::NUMBER)),
                                  EXEC_DISPATCH(&PointDivision::divide_num))));

    FUNC_BODY(PointDivision, divide_num)
    {
      const Point& coord = args.front()->as<PointAdapter>().get_object();
      const float n = args.back()->as<Lisple::Number>().float_value();

      return PointAdapter::make<Point>(coord.x / n, coord.y / n);
    }

    /* PointPlus */
    FUNC_IMPL(PointPlus, SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                              EXEC_DISPATCH(&PointPlus::plus))));

    FUNC_BODY(PointPlus, plus)
    {
      return PointAdapter::make<Point>(args.front()->as<PointAdapter>().get_object() +
                                       args.back()->as<PointAdapter>().get_object());
    }

    /* PointMinus */
    FUNC_IMPL(PointMinus, SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                               EXEC_DISPATCH(&PointMinus::minus))));

    FUNC_BODY(PointMinus, minus)
    {
      return PointAdapter::make<Point>(args.front()->as<PointAdapter>().get_object() -
                                       args.back()->as<PointAdapter>().get_object());
    }

  } // namespace Function

  /* PointAdapter */
  HOST_ADAPTER_IMPL(PointAdapter, Point, &HostType::POINT,
                    ({K_GET_SET(PointAdapter, MapKey::X, x),
                      K_GET_SET(PointAdapter, MapKey::Y, y)}));

  ADAPTER_PROP_GET_SET__FIELD(PointAdapter, x, Lisple::Number, x);
  ADAPTER_PROP_GET_SET__FIELD(PointAdapter, y, Lisple::Number, y);

  PointNamespace::PointNamespace()
      : Lisple::Namespace(std::string(NS__PIXILS__POINT))
  {
    objects.emplace(FN__DISTANCE, std::make_shared<Function::DistanceBetween>());
    objects.emplace(FN__DIVIDE, std::make_shared<Function::PointDivision>());
    objects.emplace(FN__MAKE_POINT, std::make_shared<Function::MakePoint>());
    objects.emplace(FN__MINUS, std::make_shared<Function::PointMinus>());
    objects.emplace(FN__PLUS, std::make_shared<Function::PointPlus>());
    objects.emplace(FN__ROTATE, std::make_shared<Function::RotatePoint>());
  }

} // namespace Pixils::Script
