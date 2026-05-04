#include <pixils/binding/arg_collector.h>
#include <pixils/binding/color_namespace.h>

#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/host/schema.h>
#include <lisple/host/transform.h>
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

  namespace Function
  {
    /* ColorAdapter make function */
    FUNC_IMPL(MakeColor,
              MULTI_SIG((FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakeColor::exec_make_color_map)),
                        (FN_ARGS((&Lisple::Type::STRING)),
                         EXEC_DISPATCH(&MakeColor::exec_make_color_string))));

    Lisple::MapSchema color_schema({{"r", &Lisple::Type::NUMBER},
                                    {"g", &Lisple::Type::NUMBER},
                                    {"b", &Lisple::Type::NUMBER}},
                                   {{"a", &Lisple::Type::NUMBER}});

    EXEC_BODY(MakeColor, exec_make_color_map)
    {
      auto input = color_schema.bind(ctx, *args[0]);

      return ColorAdapter::make_unique(input.ui8("r"),
                                       input.ui8("g"),
                                       input.ui8("b"),
                                       input.ui8("a", 0xFF));
    }

    EXEC_BODY(MakeColor, exec_make_color_string)
    {
      const Color color = Color::from_hex_string(args[0]->str());

      return ColorAdapter::make_unique(color.r, color.g, color.b, color.a);
    }

    /* WithAlpha - with-alpha */
    FUNC_IMPL(WithAlpha,
              SIG((FN_ARGS((&HostType::COLOR), (&Lisple::Type::NUMBER)),
                   EXEC_DISPATCH(&WithAlpha::exec_with_alpha))));

    EXEC_BODY(WithAlpha, exec_with_alpha)
    {
      const Color& source = Lisple::obj<Color>(*args[0]);

      return ColorAdapter::make_unique(source.r, source.g, source.b, args[1]->ui8());
    }

  } // namespace Function

  /** ColorAdapter */
  NATIVE_ADAPTER_IMPL(ColorAdapter, Color, &HostType::COLOR, (r), (g), (b), (a));

  NOBJ_PROP_GET_SET__FIELD(ColorAdapter, r);
  NOBJ_PROP_GET_SET__FIELD(ColorAdapter, g);
  NOBJ_PROP_GET_SET__FIELD(ColorAdapter, b);
  NOBJ_PROP_GET_SET__FIELD(ColorAdapter, a);

  ColorNamespace::ColorNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__COLOR))
  {
    values.emplace(FN__MAKE_COLOR, Function::MakeColor::make());
    values.emplace(FN__WITH_ALPHA, Function::WithAlpha::make());

    values.emplace("ALICE-BLUE", ColorAdapter::make_unique(0xf0, 0xf8, 0xff));
    values.emplace("ANTIQUE-WHITE", ColorAdapter::make_unique(0xfa, 0xeb, 0xd7));
    values.emplace("AQUA", ColorAdapter::make_unique(0x00, 0xff, 0xff));
    values.emplace("AQUAMARINE", ColorAdapter::make_unique(0x7f, 0xff, 0xd4));
    values.emplace("AZURE", ColorAdapter::make_unique(0xf0, 0xff, 0xff));
    values.emplace("BEIGE", ColorAdapter::make_unique(0xf5, 0xf5, 0xdc));
    values.emplace("BISQUE", ColorAdapter::make_unique(0xff, 0xe4, 0xc4));
    values.emplace("BLACK", ColorAdapter::make_unique(0x00, 0x00, 0x00));
    values.emplace("BLANCHED-ALMOND", ColorAdapter::make_unique(0xff, 0xeb, 0xcd));
    values.emplace("BLUE", ColorAdapter::make_unique(0x00, 0x00, 0xff));
    values.emplace("BLUE-VIOLET", ColorAdapter::make_unique(0x8a, 0x2b, 0xe2));
    values.emplace("BROWN", ColorAdapter::make_unique(0xa5, 0x2a, 0x2a));
    values.emplace("BURLY-WOOD", ColorAdapter::make_unique(0xde, 0xb8, 0x87));
    values.emplace("CADET-BLUE", ColorAdapter::make_unique(0x5f, 0x9e, 0xa0));
    values.emplace("CHARTREUSE", ColorAdapter::make_unique(0x7f, 0xff, 0x00));
    values.emplace("CHOCOLATE", ColorAdapter::make_unique(0xd2, 0x69, 0x1e));
    values.emplace("CORAL", ColorAdapter::make_unique(0xff, 0x7f, 0x50));
    values.emplace("CORNFLOWER-BLUE", ColorAdapter::make_unique(0x64, 0x95, 0xed));
    values.emplace("CORNSILK", ColorAdapter::make_unique(0xff, 0xf8, 0xdc));
    values.emplace("CRIMSON", ColorAdapter::make_unique(0xdc, 0x14, 0x3c));
    values.emplace("CYAN", ColorAdapter::make_unique(0x00, 0xff, 0xff));
    values.emplace("DARK-BLUE", ColorAdapter::make_unique(0x00, 0x00, 0x8b));
    values.emplace("DARK-CYAN", ColorAdapter::make_unique(0x00, 0x8b, 0x8b));
    values.emplace("DARK-GOLDENROD", ColorAdapter::make_unique(0xb8, 0x86, 0x0b));
    values.emplace("DARK-GRAY", ColorAdapter::make_unique(0xa9, 0xa9, 0xa9));
    values.emplace("DARK-GREEN", ColorAdapter::make_unique(0x00, 0x64, 0x00));
    values.emplace("DARK-GREY", ColorAdapter::make_unique(0xa9, 0xa9, 0xa9));
    values.emplace("DARK-KHAKI", ColorAdapter::make_unique(0xbd, 0xb7, 0x6b));
    values.emplace("DARK-MAGENTA", ColorAdapter::make_unique(0x8b, 0x00, 0x8b));
    values.emplace("DARK-OLIVE-GREEN", ColorAdapter::make_unique(0x55, 0x6b, 0x2f));
    values.emplace("DARK-ORANGE", ColorAdapter::make_unique(0xff, 0x8c, 0x00));
    values.emplace("DARK-ORCHID", ColorAdapter::make_unique(0x99, 0x32, 0xcc));
    values.emplace("DARK-RED", ColorAdapter::make_unique(0x8b, 0x00, 0x00));
    values.emplace("DARK-SALMON", ColorAdapter::make_unique(0xe9, 0x96, 0x7a));
    values.emplace("DARK-SEA-GREEN", ColorAdapter::make_unique(0x8f, 0xbc, 0x8f));
    values.emplace("DARK-SLATE-BLUE", ColorAdapter::make_unique(0x48, 0x3d, 0x8b));
    values.emplace("DARK-SLATE-GRAY", ColorAdapter::make_unique(0x2f, 0x4f, 0x4f));
    values.emplace("DARK-SLATE-GREY", ColorAdapter::make_unique(0x2f, 0x4f, 0x4f));
    values.emplace("DARK-TURQUOISE", ColorAdapter::make_unique(0x00, 0xce, 0xd1));
    values.emplace("DARK-VIOLET", ColorAdapter::make_unique(0x94, 0x00, 0xd3));
    values.emplace("DEEP-PINK", ColorAdapter::make_unique(0xff, 0x14, 0x93));
    values.emplace("DEEP-SKY-BLUE", ColorAdapter::make_unique(0x00, 0xbf, 0xff));
    values.emplace("DIM-GRAY", ColorAdapter::make_unique(0x69, 0x69, 0x69));
    values.emplace("DIM-GREY", ColorAdapter::make_unique(0x69, 0x69, 0x69));
    values.emplace("DODGER-BLUE", ColorAdapter::make_unique(0x1e, 0x90, 0xff));
    values.emplace("FIREBRICK", ColorAdapter::make_unique(0xb2, 0x22, 0x22));
    values.emplace("FLORAL-WHITE", ColorAdapter::make_unique(0xff, 0xfa, 0xf0));
    values.emplace("FOREST-GREEN", ColorAdapter::make_unique(0x22, 0x8b, 0x22));
    values.emplace("FUCHSIA", ColorAdapter::make_unique(0xff, 0x00, 0xff));
    values.emplace("GAINSBORO", ColorAdapter::make_unique(0xdc, 0xdc, 0xdc));
    values.emplace("GHOST-WHITE", ColorAdapter::make_unique(0xf8, 0xf8, 0xff));
    values.emplace("GOLD", ColorAdapter::make_unique(0xff, 0xd7, 0x00));
    values.emplace("GOLDENROD", ColorAdapter::make_unique(0xda, 0xa5, 0x20));
    values.emplace("GRAY", ColorAdapter::make_unique(0x80, 0x80, 0x80));
    values.emplace("GREEN", ColorAdapter::make_unique(0x00, 0x80, 0x00));
    values.emplace("GREEN-YELLOW", ColorAdapter::make_unique(0xad, 0xff, 0x2f));
    values.emplace("GREY", ColorAdapter::make_unique(0x80, 0x80, 0x80));
    values.emplace("HONEYDEW", ColorAdapter::make_unique(0xf0, 0xff, 0xf0));
    values.emplace("HOT-PINK", ColorAdapter::make_unique(0xff, 0x69, 0xb4));
    values.emplace("INDIAN-RED", ColorAdapter::make_unique(0xcd, 0x5c, 0x5c));
    values.emplace("INDIGO", ColorAdapter::make_unique(0x4b, 0x00, 0x82));
    values.emplace("IVORY", ColorAdapter::make_unique(0xff, 0xff, 0xf0));
    values.emplace("KHAKI", ColorAdapter::make_unique(0xf0, 0xe6, 0x8c));
    values.emplace("LAVENDER", ColorAdapter::make_unique(0xe6, 0xe6, 0xfa));
    values.emplace("LAVENDER-BLUSH", ColorAdapter::make_unique(0xff, 0xf0, 0xf5));
    values.emplace("LAWN-GREEN", ColorAdapter::make_unique(0x7c, 0xfc, 0x00));
    values.emplace("LEMON-CHIFFON", ColorAdapter::make_unique(0xff, 0xfa, 0xcd));
    values.emplace("LIGHT-BLUE", ColorAdapter::make_unique(0xad, 0xd8, 0xe6));
    values.emplace("LIGHT-CORAL", ColorAdapter::make_unique(0xf0, 0x80, 0x80));
    values.emplace("LIGHT-CYAN", ColorAdapter::make_unique(0xe0, 0xff, 0xff));
    values.emplace("LIGHT-GOLDENROD-YELLOW", ColorAdapter::make_unique(0xfa, 0xfa, 0xd2));
    values.emplace("LIGHT-GRAY", ColorAdapter::make_unique(0xd3, 0xd3, 0xd3));
    values.emplace("LIGHT-GREEN", ColorAdapter::make_unique(0x90, 0xee, 0x90));
    values.emplace("LIGHT-GREY", ColorAdapter::make_unique(0xd3, 0xd3, 0xd3));
    values.emplace("LIGHT-PINK", ColorAdapter::make_unique(0xff, 0xb6, 0xc1));
    values.emplace("LIGHT-SALMON", ColorAdapter::make_unique(0xff, 0xa0, 0x7a));
    values.emplace("LIGHT-SEA-GREEN", ColorAdapter::make_unique(0x20, 0xb2, 0xaa));
    values.emplace("LIGHT-SKY-BLUE", ColorAdapter::make_unique(0x87, 0xce, 0xfa));
    values.emplace("LIGHT-SLATE-GRAY", ColorAdapter::make_unique(0x77, 0x88, 0x99));
    values.emplace("LIGHT-SLATE-GREY", ColorAdapter::make_unique(0x77, 0x88, 0x99));
    values.emplace("LIGHT-STEEL-BLUE", ColorAdapter::make_unique(0xb0, 0xc4, 0xde));
    values.emplace("LIGHT-YELLOW", ColorAdapter::make_unique(0xff, 0xff, 0xe0));
    values.emplace("LIME", ColorAdapter::make_unique(0x00, 0xff, 0x00));
    values.emplace("LIME-GREEN", ColorAdapter::make_unique(0x32, 0xcd, 0x32));
    values.emplace("LINEN", ColorAdapter::make_unique(0xfa, 0xf0, 0xe6));
    values.emplace("MAGENTA", ColorAdapter::make_unique(0xff, 0x00, 0xff));
    values.emplace("MAROON", ColorAdapter::make_unique(0x80, 0x00, 0x00));
    values.emplace("MEDIUM-AQUAMARINE", ColorAdapter::make_unique(0x66, 0xcd, 0xaa));
    values.emplace("MEDIUM-BLUE", ColorAdapter::make_unique(0x00, 0x00, 0xcd));
    values.emplace("MEDIUM-ORCHID", ColorAdapter::make_unique(0xba, 0x55, 0xd3));
    values.emplace("MEDIUM-PURPLE", ColorAdapter::make_unique(0x93, 0x70, 0xdb));
    values.emplace("MEDIUM-SEA-GREEN", ColorAdapter::make_unique(0x3c, 0xb3, 0x71));
    values.emplace("MEDIUM-SLATE-BLUE", ColorAdapter::make_unique(0x7b, 0x68, 0xee));
    values.emplace("MEDIUM-SPRING-GREEN", ColorAdapter::make_unique(0x00, 0xfa, 0x9a));
    values.emplace("MEDIUM-TURQUOISE", ColorAdapter::make_unique(0x48, 0xd1, 0xcc));
    values.emplace("MEDIUM-VIOLET-RED", ColorAdapter::make_unique(0xc7, 0x15, 0x85));
    values.emplace("MIDNIGHT-BLUE", ColorAdapter::make_unique(0x19, 0x19, 0x70));
    values.emplace("MINT-CREAM", ColorAdapter::make_unique(0xf5, 0xff, 0xfa));
    values.emplace("MISTY-ROSE", ColorAdapter::make_unique(0xff, 0xe4, 0xe1));
    values.emplace("MOCCASIN", ColorAdapter::make_unique(0xff, 0xe4, 0xb5));
    values.emplace("NAVAJO-WHITE", ColorAdapter::make_unique(0xff, 0xde, 0xad));
    values.emplace("NAVY", ColorAdapter::make_unique(0x00, 0x00, 0x80));
    values.emplace("OLD-LACE", ColorAdapter::make_unique(0xfd, 0xf5, 0xe6));
    values.emplace("OLIVE", ColorAdapter::make_unique(0x80, 0x80, 0x00));
    values.emplace("OLIVE-DRAB", ColorAdapter::make_unique(0x6b, 0x8e, 0x23));
    values.emplace("ORANGE", ColorAdapter::make_unique(0xff, 0xa5, 0x00));
    values.emplace("ORANGE-RED", ColorAdapter::make_unique(0xff, 0x45, 0x00));
    values.emplace("ORCHID", ColorAdapter::make_unique(0xda, 0x70, 0xd6));
    values.emplace("PALE-GOLDENROD", ColorAdapter::make_unique(0xee, 0xe8, 0xaa));
    values.emplace("PALE-GREEN", ColorAdapter::make_unique(0x98, 0xfb, 0x98));
    values.emplace("PALE-TURQUOISE", ColorAdapter::make_unique(0xaf, 0xee, 0xee));
    values.emplace("PALE-VIOLET-RED", ColorAdapter::make_unique(0xdb, 0x70, 0x93));
    values.emplace("PAPAYA-WHIP", ColorAdapter::make_unique(0xff, 0xef, 0xd5));
    values.emplace("PEACH-PUFF", ColorAdapter::make_unique(0xff, 0xda, 0xb9));
    values.emplace("PERU", ColorAdapter::make_unique(0xcd, 0x85, 0x3f));
    values.emplace("PINK", ColorAdapter::make_unique(0xff, 0xc0, 0xcb));
    values.emplace("PLUM", ColorAdapter::make_unique(0xdd, 0xa0, 0xdd));
    values.emplace("POWDER-BLUE", ColorAdapter::make_unique(0xb0, 0xe0, 0xe6));
    values.emplace("PURPLE", ColorAdapter::make_unique(0x80, 0x00, 0x80));
    values.emplace("REBECCA-PURPLE", ColorAdapter::make_unique(0x66, 0x33, 0x99));
    values.emplace("RED", ColorAdapter::make_unique(0xff, 0x00, 0x00));
    values.emplace("ROSY-BROWN", ColorAdapter::make_unique(0xbc, 0x8f, 0x8f));
    values.emplace("ROYAL-BLUE", ColorAdapter::make_unique(0x41, 0x69, 0xe1));
    values.emplace("SADDLE-BROWN", ColorAdapter::make_unique(0x8b, 0x45, 0x13));
    values.emplace("SALMON", ColorAdapter::make_unique(0xfa, 0x80, 0x72));
    values.emplace("SANDY-BROWN", ColorAdapter::make_unique(0xf4, 0xa4, 0x60));
    values.emplace("SEA-GREEN", ColorAdapter::make_unique(0x2e, 0x8b, 0x57));
    values.emplace("SEA-SHELL", ColorAdapter::make_unique(0xff, 0xf5, 0xee));
    values.emplace("SIENNA", ColorAdapter::make_unique(0xa0, 0x52, 0x2d));
    values.emplace("SILVER", ColorAdapter::make_unique(0xc0, 0xc0, 0xc0));
    values.emplace("SKY-BLUE", ColorAdapter::make_unique(0x87, 0xce, 0xeb));
    values.emplace("SLATE-BLUE", ColorAdapter::make_unique(0x6a, 0x5a, 0xcd));
    values.emplace("SLATE-GRAY", ColorAdapter::make_unique(0x70, 0x80, 0x90));
    values.emplace("SLATE-GREY", ColorAdapter::make_unique(0x70, 0x80, 0x90));
    values.emplace("SNOW", ColorAdapter::make_unique(0xff, 0xfa, 0xfa));
    values.emplace("SPRING-GREEN", ColorAdapter::make_unique(0x00, 0xff, 0x7f));
    values.emplace("STEEL-BLUE", ColorAdapter::make_unique(0x46, 0x82, 0xb4));
    values.emplace("TAN", ColorAdapter::make_unique(0xd2, 0xb4, 0x8c));
    values.emplace("TEAL", ColorAdapter::make_unique(0x00, 0x80, 0x80));
    values.emplace("THISTLE", ColorAdapter::make_unique(0xd8, 0xbf, 0xd8));
    values.emplace("TOMATO", ColorAdapter::make_unique(0xff, 0x63, 0x47));
    values.emplace("TURQUOISE", ColorAdapter::make_unique(0x40, 0xe0, 0xd0));
    values.emplace("VIOLET", ColorAdapter::make_unique(0xee, 0x82, 0xee));
    values.emplace("WHEAT", ColorAdapter::make_unique(0xf5, 0xde, 0xb3));
    values.emplace("WHITE", ColorAdapter::make_unique(0xff, 0xff, 0xff));
    values.emplace("WHITE-SMOKE", ColorAdapter::make_unique(0xf5, 0xf5, 0xf5));
    values.emplace("YELLOW", ColorAdapter::make_unique(0xff, 0xff, 0x00));
    values.emplace("YELLOW-GREEN", ColorAdapter::make_unique(0x9a, 0xcd, 0x32));
    values.emplace("TRANSPARENT", ColorAdapter::make_unique(0x00, 0x00, 0x00, 0x00));
  }

} // namespace Pixils::Script
