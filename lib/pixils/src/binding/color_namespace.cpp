
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/color_namespace.h>

#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/host/schema.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(A, "a");
    SHKEY(B, "b");
    SHKEY(G, "g");
    SHKEY(R, "r");
  } // namespace MapKey

  /* ColorAdapter */
  HOST_ADAPTER_IMPL(ColorAdapter,
                    Color,
                    &HostType::COLOR,
                    ({K_GET_SET(ColorAdapter, MapKey::R, r),
                      K_GET_SET(ColorAdapter, MapKey::G, g),
                      K_GET_SET(ColorAdapter, MapKey::B, b),
                      K_GET_SET(ColorAdapter, MapKey::A, a)}));

  ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, r, Lisple::Number);
  ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, g, Lisple::Number);
  ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, b, Lisple::Number);
  ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, a, Lisple::Number);

  namespace Function
  {
    /* ColorAdapter make function */
    FUNC_IMPL(MakeColor,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeColor::exec_make_color))));

    Lisple::MapSchema color_schema({{"r", &Lisple::Type::NUMBER},
                                    {"g", &Lisple::Type::NUMBER},
                                    {"b", &Lisple::Type::NUMBER}},
                                   {{"a", &Lisple::Type::NUMBER}});

    EXEC_BODY(MakeColor, exec_make_color)
    {
      auto input = color_schema.bind(ctx, *args[0]);

      std::unique_ptr<Color> color = std::make_unique<Color>();
      color->r = input.ui8("r");
      color->g = input.ui8("g");
      color->b = input.ui8("b");
      color->a = input.ui8("a", 0xff);

      return Lisple::RTValue::object(std::make_shared<ColorAdapter>(std::move(color)));
    }

    /* WithAlpha - with-alpha */
    FUNC_IMPL(WithAlpha,
              SIG((FN_ARGS((&HostType::COLOR), (&Lisple::Type::NUMBER)),
                   EXEC_DISPATCH(&WithAlpha::exec_with_alpha))));

    EXEC_BODY(WithAlpha, exec_with_alpha)
    {
      const Color& source = Lisple::obj<Color>(*args[0]);

      std::unique_ptr<Color> color = std::make_unique<Color>(source);
      color->a = args[1]->ui8();

      return Lisple::RTValue::object(std::make_shared<ColorAdapter>(std::move(color)));
    }

  } // namespace Function

  ColorNamespace::ColorNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__COLOR))
  {
    values.emplace(FN__MAKE_COLOR, Function::MakeColor::make());
    objects.emplace(FN__WITH_ALPHA, std::make_shared<Function::WithAlpha>());
  }

} // namespace Pixils::Script
