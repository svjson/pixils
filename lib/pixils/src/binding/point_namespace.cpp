
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/geom.h>

#include <lisple/exception.h>
#include <lisple/host/object.h>
#include <lisple/host/schema.h>

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
    FUNC_IMPL(MakePoint,
              MULTI_SIG((FN_ARGS((&Lisple::Type::NUMBER), (&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MakePoint::exec_point_from_ints)),
                        (FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakePoint::exec_point_from_map))));

    Lisple::MapSchema point_schema({{"x", &Lisple::Type::NUMBER},
                                    {"y", &Lisple::Type::NUMBER}});

    EXEC_BODY(MakePoint, exec_point_from_map)
    {
      if (*args[0] == *Lisple::Constant::NIL)
      {
        throw Lisple::TypeError("Cannot create Point from nil");
      }

      auto input = point_schema.bind(ctx, *args[0]);
      return PointAdapter::make_unique(input.f32("x"), input.f32("y"));
    }

    EXEC_BODY(MakePoint, exec_point_from_ints)
    {
      return PointAdapter::make_unique(args.at(0)->f32(), args.at(1)->f32());
    }

    /* Distance Between - distance-between */
    FUNC_IMPL(DistanceBetween,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&DistanceBetween::exec_distance))))

    EXEC_BODY(DistanceBetween, exec_distance)
    {
      const Point& a = Lisple::obj<Point>(*args[0]);
      const Point& b = Lisple::obj<Point>(*args[1]);

      return Lisple::RTValue::number(a.distance_to(b));
    }

    /* Rotate Point - rotate-point */
    FUNC_IMPL(
      RotatePoint,
      MULTI_SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT), (&Lisple::Type::NUMBER)),
                 EXEC_DISPATCH(&RotatePoint::exec_orig_amount)),
                (FN_ARGS((&HostType::POINT), (&Lisple::Type::NUMBER)),
                 EXEC_DISPATCH(&RotatePoint::exec_amount)),
                (FN_ARGS((&HostType::POINT), (&Lisple::Type::MAP)),
                 EXEC_DISPATCH(&RotatePoint::exec_with_opts))))

    Lisple::MapSchema rotate_opts_schema({},
                                         {{"origin", &HostType::POINT},
                                          {"radians", &Lisple::Type::NUMBER}});

    EXEC_BODY(RotatePoint, exec_with_opts)
    {
      const Point& point = Lisple::obj<Point>(*args[0]);
      auto map = rotate_opts_schema.bind(ctx, *args[1]);

      return PointAdapter::make_unique(
        point.rotate(map.obj<Point>("origin", POINT__ZERO_ZERO), map.f32("radians", 0.0f)));
    }

    EXEC_BODY(RotatePoint, exec_amount)
    {
      Lisple::sptr_rtval_v fwd_args = {
        args[0],
        Lisple::RTValue::map({Lisple::RTValue::keyword("radians"), args[1]})};

      return this->exec_with_opts(ctx, fwd_args);
    }

    EXEC_BODY(RotatePoint, exec_orig_amount)
    {
      Lisple::sptr_rtval_v fwd_args = {
        args[0],
        Lisple::RTValue::map({Lisple::RTValue::keyword("origin"),
                              args[1],
                              Lisple::RTValue::keyword("amount"),
                              args[2]})};

      return this->exec_with_opts(ctx, fwd_args);
    }

    /* PointMultiplication */
    FUNC_IMPL(PointMultiplication,
              SIG((FN_ARGS((&HostType::POINT), (&Lisple::Type::NUMBER)),
                   EXEC_DISPATCH(&PointMultiplication::exec_multiply_num))));

    EXEC_BODY(PointMultiplication, exec_multiply_num)
    {
      const Point& coord = Lisple::obj<Point>(*args.front());
      const float n = args.back()->f32();

      return PointAdapter::make_unique(coord.x * n, coord.y * n);
    }

    /* PointDivision */
    FUNC_IMPL(PointDivision,
              SIG((FN_ARGS((&HostType::POINT), (&Lisple::Type::NUMBER)),
                   EXEC_DISPATCH(&PointDivision::exec_divide_num))));

    EXEC_BODY(PointDivision, exec_divide_num)
    {
      const Point& coord = Lisple::obj<Point>(*args.front());
      const float n = args.back()->f32();

      return PointAdapter::make_unique(coord.x / n, coord.y / n);
    }

    /* PointPlus */
    FUNC_IMPL(PointPlus,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&PointPlus::exec_plus))));

    EXEC_BODY(PointPlus, exec_plus)
    {
      return PointAdapter::make_unique(Lisple::obj<Point>(*args.front()) +
                                       Lisple::obj<Point>(*args.back()));
    }

    /* PointMinus */
    FUNC_IMPL(PointMinus,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&PointMinus::exec_minus))));

    EXEC_BODY(PointMinus, exec_minus)
    {
      return PointAdapter::make_unique(Lisple::obj<Point>(*args.front()) -
                                       Lisple::obj<Point>(*args.back()));
    }

  } // namespace Function

  /* PointAdapter */
  NATIVE_ADAPTER_IMPL(PointAdapter, Point, &HostType::POINT, (x), (y));

  NOBJ_PROP_GET_SET__FIELD(PointAdapter, x);
  NOBJ_PROP_GET_SET__FIELD(PointAdapter, y);

  PointNamespace::PointNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__POINT))
  {
    values.emplace(FN__DISTANCE, Function::DistanceBetween::make());
    values.emplace(FN__DIVIDE, Function::PointDivision::make());
    values.emplace(FN__MAKE_POINT, Function::MakePoint::make());
    values.emplace(FN__MINUS, Function::PointMinus::make());
    values.emplace(FN__MULTIPLY, Function::PointMultiplication::make());
    values.emplace(FN__PLUS, Function::PointPlus::make());
    values.emplace(FN__ROTATE, Function::RotatePoint::make());
  }

} // namespace Pixils::Script
