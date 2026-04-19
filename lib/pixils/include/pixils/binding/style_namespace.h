
#ifndef PIXILS__STYLE_NAMESPACE_H
#define PIXILS__STYLE_NAMESPACE_H

#include <pixils/ui/style.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  inline const std::string_view NS__PIXILS__UI__STYLE = "pixils.ui.style";

  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_STYLE =
    std::string_view("pixils.ui.style/make-style");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_PADDING =
    std::string_view("pixils.ui.style/make-padding");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_BACKGROUND =
    std::string_view("pixils.ui.style/make-background");

  namespace HostType
  {
    HOST_TYPE(STYLE, "HStyle", std::string(FN__PIXILS_UI_STYLE__MAKE_STYLE));
    HOST_TYPE(STYLE_BACKGROUND,
              "HStyleBackground",
              std::string(FN__PIXILS_UI_STYLE__MAKE_BACKGROUND));
    HOST_TYPE(STYLE_PADDING,
              "HStylePadding",
              std::string(FN__PIXILS_UI_STYLE__MAKE_PADDING));
  } // namespace HostType

  namespace Function
  {
    FUNC(MakeStyle, make);
    FUNC(MakeBackground, make_color, make_image, make_map);
    FUNC(MakePadding, make_num, make_map, make_vec);

  } // namespace Function

  NATIVE_ADAPTER(StyleAdapter,
                 UI::Style,
                 (background, padding, width, height, position, top, left, direction, hover));
  NATIVE_ADAPTER(BackgroundAdapter, UI::Style::Background, (color, image));
  NATIVE_ADAPTER(PaddingAdapter, UI::Style::Padding, (t, r, b, l));

  class StyleNamespace : public Lisple::Namespace
  {
   public:
    StyleNamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__STYLE_NAMESPACE_H */
