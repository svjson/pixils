
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/geom.h>

#include <algorithm>
#include <roo/exception.h>
#include <roo/host/object.h>
#include <roo/host/schema.h>

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
    /** IntPointFunction - int */
    FUNC_IMPL(IntPointFunction,
              SIG((FN_ARGS((&HostType::POINT)), EXEC_DISPATCH(&IntPointFunction::exec_int))))

    EXEC_BODY(IntPointFunction, exec_int)
    {
      return PointAdapter::make_unique(Roo::obj<Point>(*args[0]).floor());
    }

    /* Point make-function */
    FUNC_IMPL(MakePoint,
              MULTI_SIG((FN_ARGS((&Roo::Type::NUMBER), (&Roo::Type::NUMBER)),
                         EXEC_DISPATCH(&MakePoint::exec_point_from_ints)),
                        (FN_ARGS((&Roo::Type::MAP)),
                         EXEC_DISPATCH(&MakePoint::exec_point_from_map))));

    Roo::MapSchema point_schema({{"x", &Roo::Type::NUMBER}, {"y", &Roo::Type::NUMBER}});

    EXEC_BODY(MakePoint, exec_point_from_map)
    {
      if (*args[0] == *Roo::Constant::NIL)
      {
        throw Roo::TypeError("Cannot create Point from nil");
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
      const Point& a = Roo::obj<Point>(*args[0]);
      const Point& b = Roo::obj<Point>(*args[1]);

      return Roo::number(a.distance_to(b));
    }

    /* Distance Squared - distance-squared */
    FUNC_IMPL(DistanceSquared,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&DistanceSquared::exec_distance_squared))))

    EXEC_BODY(DistanceSquared, exec_distance_squared)
    {
      const Point& a = Roo::obj<Point>(*args[0]);
      const Point& b = Roo::obj<Point>(*args[1]);

      return Roo::number(a.distance_squared_to(b));
    }

    /* Clamp Point */
    FUNC_IMPL(ClampPoint,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::RECT)),
                   EXEC_DISPATCH(&ClampPoint::exec_clamp))));

    EXEC_BODY(ClampPoint, exec_clamp)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      const Rect& rect = Roo::obj<Rect>(*args[1]);

      const float x = std::max(static_cast<float>(rect.x),
                               std::min(point.x, static_cast<float>(rect.x + rect.w)));
      const float y = std::max(static_cast<float>(rect.y),
                               std::min(point.y, static_cast<float>(rect.y + rect.h)));

      return PointAdapter::make_unique(x, y);
    }

    /* Translate Point */
    FUNC_IMPL(TranslatePoint,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&TranslatePoint::exec_translate))));

    EXEC_BODY(TranslatePoint, exec_translate)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      return PointAdapter::make_unique(point.plus(args[1]->f32(), args[2]->f32()));
    }

    /* Translate Point X */
    FUNC_IMPL(TranslatePointX,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&TranslatePointX::exec_translate_x))));

    EXEC_BODY(TranslatePointX, exec_translate_x)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      return PointAdapter::make_unique(point.plus(args[1]->f32(), 0));
    }

    /* Translate Point Y */
    FUNC_IMPL(TranslatePointY,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&TranslatePointY::exec_translate_y))));

    EXEC_BODY(TranslatePointY, exec_translate_y)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      return PointAdapter::make_unique(point.plus(0, args[1]->f32()));
    }

    /* Wrap Point */
    FUNC_IMPL(WrapPoint,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::RECT)),
                   EXEC_DISPATCH(&WrapPoint::exec_wrap))));

    EXEC_BODY(WrapPoint, exec_wrap)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      const Rect& rect = Roo::obj<Rect>(*args[1]);

      const float min_x = rect.x;
      const float max_x = rect.x + rect.w;
      const float min_y = rect.y;
      const float max_y = rect.y + rect.h;

      const float x = point.x < min_x ? max_x : point.x > max_x ? min_x : point.x;
      const float y = point.y < min_y ? max_y : point.y > max_y ? min_y : point.y;

      return PointAdapter::make_unique(x, y);
    }

    /* Rotate Point - rotate-point */
    FUNC_IMPL(
      RotatePoint,
      MULTI_SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT), (&Roo::Type::NUMBER)),
                 EXEC_DISPATCH(&RotatePoint::exec_orig_amount)),
                (FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                 EXEC_DISPATCH(&RotatePoint::exec_amount)),
                (FN_ARGS((&HostType::POINT), (&Roo::Type::MAP)),
                 EXEC_DISPATCH(&RotatePoint::exec_with_opts))))

    Roo::MapSchema rotate_opts_schema({},
                                      {{"origin", &HostType::POINT},
                                       {"radians", &Roo::Type::NUMBER}});

    EXEC_BODY(RotatePoint, exec_with_opts)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      auto map = rotate_opts_schema.bind(ctx, *args[1]);

      return PointAdapter::make_unique(
        point.rotate(map.obj<Point>("origin", POINT__ZERO_ZERO), map.f32("radians", 0.0f)));
    }

    EXEC_BODY(RotatePoint, exec_amount)
    {
      Roo::sptr_val_v fwd_args = {args[0], Roo::map({Roo::keyword("radians"), args[1]})};

      return this->exec_with_opts(ctx, fwd_args);
    }

    EXEC_BODY(RotatePoint, exec_orig_amount)
    {
      Roo::sptr_val_v fwd_args = {
        args[0],
        Roo::map({Roo::keyword("origin"), args[1], Roo::keyword("amount"), args[2]})};

      return this->exec_with_opts(ctx, fwd_args);
    }

    /* PointMultiplication */
    FUNC_IMPL(PointMultiplication,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&PointMultiplication::exec_multiply_num))));

    EXEC_BODY(PointMultiplication, exec_multiply_num)
    {
      const Point& coord = Roo::obj<Point>(*args.front());
      const float n = args.back()->f32();

      return PointAdapter::make_unique(coord.x * n, coord.y * n);
    }

    /* PointDivision */
    FUNC_IMPL(PointDivision,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&PointDivision::exec_divide_num))));

    EXEC_BODY(PointDivision, exec_divide_num)
    {
      const Point& coord = Roo::obj<Point>(*args.front());
      const float n = args.back()->f32();

      return PointAdapter::make_unique(coord.x / n, coord.y / n);
    }

    /* PointPlus */
    FUNC_IMPL(PointPlus,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&PointPlus::exec_plus))));

    EXEC_BODY(PointPlus, exec_plus)
    {
      return PointAdapter::make_unique(Roo::obj<Point>(*args.front()) +
                                       Roo::obj<Point>(*args.back()));
    }

    /* PointMinus */
    FUNC_IMPL(PointMinus,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&PointMinus::exec_minus))));

    EXEC_BODY(PointMinus, exec_minus)
    {
      return PointAdapter::make_unique(Roo::obj<Point>(*args.front()) -
                                       Roo::obj<Point>(*args.back()));
    }

  } // namespace Function

  /* PointAdapter */
  NATIVE_ADAPTER_IMPL(PointAdapter, Point, &HostType::POINT, (x), (y));

  NOBJ_PROP_GET_SET__FIELD(PointAdapter, x);
  NOBJ_PROP_GET_SET__FIELD(PointAdapter, y);

  PointNamespace::PointNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__POINT))
  {
    values.emplace(FN__DISTANCE, Function::DistanceBetween::make());
    values.emplace(FN__DISTANCE_SQUARED, Function::DistanceSquared::make());
    values.emplace(FN__CLAMP, Function::ClampPoint::make());
    values.emplace(FN__DIVIDE, Function::PointDivision::make());
    values.emplace(FN__INT_POINT, Function::IntPointFunction::make());
    values.emplace(FN__MAKE_POINT, Function::MakePoint::make());
    values.emplace(FN__MINUS, Function::PointMinus::make());
    values.emplace(FN__MULTIPLY, Function::PointMultiplication::make());
    values.emplace(FN__PLUS, Function::PointPlus::make());
    values.emplace(FN__ROTATE, Function::RotatePoint::make());
    values.emplace(FN__TRANSLATE, Function::TranslatePoint::make());
    values.emplace(FN__TRANSLATE_X, Function::TranslatePointX::make());
    values.emplace(FN__TRANSLATE_Y, Function::TranslatePointY::make());
    values.emplace(FN__WRAP, Function::WrapPoint::make());
  }

} // namespace Pixils::Script
