
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/color_namespace.h>

#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils
{
  namespace Script
  {
    namespace MapKey
    {
      SHKEY(A, "a");
      SHKEY(B, "b");
      SHKEY(G, "g");
      SHKEY(R, "r");
    } // namespace MapKey

    /* ColorAdapter */
    HOST_ADAPTER_IMPL(
        ColorAdapter, Color, &HostType::COLOR,
        ({K_GET_SET(ColorAdapter, MapKey::R, r), K_GET_SET(ColorAdapter, MapKey::G, g),
          K_GET_SET(ColorAdapter, MapKey::B, b), K_GET_SET(ColorAdapter, MapKey::A, a)}));

    ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, r, Lisple::Number);
    ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, g, Lisple::Number);
    ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, b, Lisple::Number);
    ADAPTER_PROP_GET_SET__FIELD(ColorAdapter, a, Lisple::Number);

    namespace Function
    {
      /* ColorAdapter make function */
      FUNC_IMPL(MakeColor,
                SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeColor::make_color))));

      ArgCollector color_collector(FN__MAKE_COLOR,
                                   {{*MapKey::R, &Lisple::Type::NUMBER},
                                    {*MapKey::G, &Lisple::Type::NUMBER},
                                    {*MapKey::B, &Lisple::Type::NUMBER}},
                                   {{*MapKey::A, &Lisple::Type::NUMBER}});

      FUNC_BODY(MakeColor, make_color)
      {
        str_key_map_t keys = color_collector.collect_keys(*args.front());

        std::unique_ptr<Color> color = std::make_unique<Color>();
        color->r = ArgCollector::uint8_value(keys, *MapKey::R);
        color->g = ArgCollector::uint8_value(keys, *MapKey::G);
        color->b = ArgCollector::uint8_value(keys, *MapKey::B);
        color->a = ArgCollector::uint8_value(keys, *MapKey::A, 0xff);

        return std::make_shared<ColorAdapter>(std::move(color));
      }

      /* WithAlpha - with-alpha */
      FUNC_IMPL(WithAlpha, SIG((FN_ARGS((&HostType::COLOR), (&Lisple::Type::NUMBER)),
                                EXEC_DISPATCH(&WithAlpha::with_alpha))));

      FUNC_BODY(WithAlpha, with_alpha)
      {
        const Color& source = args[0]->as<ColorAdapter>().get_object();
        int alpha = args[1]->as<Lisple::Number>().int_value();

        std::unique_ptr<Color> color = std::make_unique<Color>(source);
        color->a = alpha;

        return std::make_shared<ColorAdapter>(std::move(color));
      }

    } // namespace Function

    ColorNamespace::ColorNamespace()
        : Lisple::Namespace(NS__PIXILS__COLOR)
    {
      objects.emplace(FN__MAKE_COLOR, std::make_shared<Function::MakeColor>());
      objects.emplace(FN__WITH_ALPHA, std::make_shared<Function::WithAlpha>());
    }

  } // namespace Script

} // namespace Pixils
