
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/geom.h>

#include <algorithm>
#include <roo/exception.h>
#include <roo/host/object.h>
#include <roo/host/schema.h>

namespace Pixils::Script
{
  namespace
  {
    Point point_bound_from_value(const Roo::sptr_val& value, const std::string& name)
    {
      if (!value || value->type == Roo::Value::Type::NIL)
      {
        throw Roo::TypeError(name + " bound cannot be nil");
      }

      if (value->type == Roo::Value::Type::NUMBER)
      {
        const float bound = value->f32();
        return Point{bound, bound};
      }

      return Roo::obj<Point>(*value);
    }

    Point clamp_point_to_bounds(const Point& point, const Point& min, const Point& max)
    {
      return Point{std::max(min.x, std::min(point.x, max.x)),
                   std::max(min.y, std::min(point.y, max.y))};
    }
  } // namespace

  namespace MapKey
  {
    SHKEY(ORIGIN, "origin");
    SHKEY(RADIANS, "radians");
    SHKEY(X, "x");
    SHKEY(Y, "y");
  } // namespace MapKey

  namespace Function
  {
    /** IntPointFunction - pixils.point/int */
    FUNC_IMPL(IntPointFunction,
              SIG((FN_ARGS((&HostType::POINT)), EXEC_DISPATCH(&IntPointFunction::exec_int))))

    EXEC_BODY(IntPointFunction, exec_int)
    {
      return PointAdapter::make_unique(Roo::obj<Point>(*args[0]).floor());
    }

    /** MakePoint - pixils.point/make-point */
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

    /** DistanceBetween - pixils.point/distance */
    FUNC_IMPL(DistanceBetween,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&DistanceBetween::exec_distance))))

    EXEC_BODY(DistanceBetween, exec_distance)
    {
      const Point& a = Roo::obj<Point>(*args[0]);
      const Point& b = Roo::obj<Point>(*args[1]);

      return Roo::number(a.distance_to(b));
    }

    /** DistanceSquared - pixils.point/distance-squared */
    FUNC_IMPL(DistanceSquared,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&DistanceSquared::exec_distance_squared))))

    EXEC_BODY(DistanceSquared, exec_distance_squared)
    {
      const Point& a = Roo::obj<Point>(*args[0]);
      const Point& b = Roo::obj<Point>(*args[1]);

      return Roo::number(a.distance_squared_to(b));
    }

    /** PointMinimum - pixils.point/min */
    FUNC_IMPL(PointMinimum,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::NUMBER_OR_POINT)),
                   EXEC_DISPATCH(&PointMinimum::exec_point_min))));

    EXEC_BODY(PointMinimum, exec_point_min)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      const Point bound = point_bound_from_value(args[1], "Upper");

      return PointAdapter::make_unique(std::min(point.x, bound.x),
                                       std::min(point.y, bound.y));
    }

    /** PointMaximum - pixils.point/max */
    FUNC_IMPL(PointMaximum,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::NUMBER_OR_POINT)),
                   EXEC_DISPATCH(&PointMaximum::exec_point_max))));

    EXEC_BODY(PointMaximum, exec_point_max)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      const Point bound = point_bound_from_value(args[1], "Lower");

      return PointAdapter::make_unique(std::max(point.x, bound.x),
                                       std::max(point.y, bound.y));
    }

    /** ClampPoint - pixils.point/clamp */
    FUNC_IMPL(ClampPoint,
              MULTI_SIG((FN_ARGS((&HostType::POINT), (&HostType::RECT)),
                         EXEC_DISPATCH(&ClampPoint::exec_clamp_rect)),
                        (FN_ARGS((&HostType::POINT),
                                 (&HostType::NUMBER_OR_POINT),
                                 (&HostType::NUMBER_OR_POINT)),
                         EXEC_DISPATCH(&ClampPoint::exec_clamp_bounds))));

    EXEC_BODY(ClampPoint, exec_clamp_rect)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      const Rect& rect = Roo::obj<Rect>(*args[1]);

      const Point min{static_cast<float>(rect.x), static_cast<float>(rect.y)};
      const Point max{static_cast<float>(rect.x + rect.w),
                      static_cast<float>(rect.y + rect.h)};

      return PointAdapter::make_unique(clamp_point_to_bounds(point, min, max));
    }

    EXEC_BODY(ClampPoint, exec_clamp_bounds)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      const Point min = point_bound_from_value(args[1], "Minimum");
      const Point max = point_bound_from_value(args[2], "Maximum");

      return PointAdapter::make_unique(clamp_point_to_bounds(point, min, max));
    }

    /** TranslatePoint - pixils.point/translate */
    FUNC_IMPL(TranslatePoint,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&TranslatePoint::exec_translate))));

    EXEC_BODY(TranslatePoint, exec_translate)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      return PointAdapter::make_unique(point.plus(args[1]->f32(), args[2]->f32()));
    }

    /** TranslatePointX - pixils.point/translate-x */
    FUNC_IMPL(TranslatePointX,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&TranslatePointX::exec_translate_x))));

    EXEC_BODY(TranslatePointX, exec_translate_x)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      return PointAdapter::make_unique(point.plus(args[1]->f32(), 0));
    }

    /** TranslatePointY - pixils.point/translate-y */
    FUNC_IMPL(TranslatePointY,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&TranslatePointY::exec_translate_y))));

    EXEC_BODY(TranslatePointY, exec_translate_y)
    {
      const Point& point = Roo::obj<Point>(*args[0]);
      return PointAdapter::make_unique(point.plus(0, args[1]->f32()));
    }

    /** WrapPoint - pixils.point/wrap */
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

    /** RotatePoint - pixils.point/rotate */
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

    // PointMultiplication - pixils.point/*
    FUNC_IMPL(PointMultiplication,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&PointMultiplication::exec_multiply_num))));

    EXEC_BODY(PointMultiplication, exec_multiply_num)
    {
      const Point& coord = Roo::obj<Point>(*args.front());
      const float n = args.back()->f32();

      return PointAdapter::make_unique(coord.x * n, coord.y * n);
    }

    /** PointDivision - pixils.point/div */
    FUNC_IMPL(PointDivision,
              SIG((FN_ARGS((&HostType::POINT), (&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&PointDivision::exec_divide_num))));

    EXEC_BODY(PointDivision, exec_divide_num)
    {
      const Point& coord = Roo::obj<Point>(*args.front());
      const float n = args.back()->f32();

      return PointAdapter::make_unique(coord.x / n, coord.y / n);
    }

    /** PointEquality - pixils.point/= */
    FUNC_IMPL(PointEquality,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&PointEquality::exec_equal))))

    EXEC_BODY(PointEquality, exec_equal)
    {
      return Roo::obj<Point>(*args.front()) == Roo::obj<Point>(*args.back())
               ? Roo::Constant::BOOL_TRUE
               : Roo::Constant::BOOL_FALSE;
    }

    /** PointPlus - pixils.point/+ */
    FUNC_IMPL(PointPlus,
              SIG((FN_ARGS((&HostType::POINT), (&HostType::POINT)),
                   EXEC_DISPATCH(&PointPlus::exec_plus))));

    EXEC_BODY(PointPlus, exec_plus)
    {
      return PointAdapter::make_unique(Roo::obj<Point>(*args.front()) +
                                       Roo::obj<Point>(*args.back()));
    }

    /** PointMinus - pixils.point/- */
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
    values.emplace(FN__EQUAL, Function::PointEquality::make());
    values.emplace(FN__INT_POINT, Function::IntPointFunction::make());
    values.emplace(FN__MAKE_POINT, Function::MakePoint::make());
    values.emplace(FN__MAX, Function::PointMaximum::make());
    values.emplace(FN__MIN, Function::PointMinimum::make());
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
