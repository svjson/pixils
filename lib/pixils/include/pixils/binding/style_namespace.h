
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

  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_BORDER =
    std::string_view("pixils.ui.style/make-border");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_BORDER_STYLE =
    std::string_view("pixils.ui.style/make-border-style");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_STYLE =
    std::string_view("pixils.ui.style/make-style");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_LAYOUT =
    std::string_view("pixils.ui.style/make-layout");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_LAYOUT_GAP =
    std::string_view("pixils.ui.style/make-layout-gap");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_INSETS =
    std::string_view("pixils.ui.style/make-insets");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_BACKGROUND =
    std::string_view("pixils.ui.style/make-background");

  namespace HostType
  {
    HOST_TYPE(BORDER, "HBorder", std::string(FN__PIXILS_UI_STYLE__MAKE_BORDER));
    HOST_TYPE(BORDER_STYLE,
              "HBorderStyle",
              std::string(FN__PIXILS_UI_STYLE__MAKE_BORDER_STYLE));
    HOST_TYPE(STYLE, "HStyle", std::string(FN__PIXILS_UI_STYLE__MAKE_STYLE));
    HOST_TYPE(STYLE_LAYOUT, "HStyleLayout", std::string(FN__PIXILS_UI_STYLE__MAKE_LAYOUT));
    HOST_TYPE(STYLE_LAYOUT_GAP,
              "HStyleLayoutGap",
              std::string(FN__PIXILS_UI_STYLE__MAKE_LAYOUT_GAP));
    HOST_TYPE(STYLE_BACKGROUND,
              "HStyleBackground",
              std::string(FN__PIXILS_UI_STYLE__MAKE_BACKGROUND));
    HOST_TYPE(STYLE_INSETS, "HStyleInsets", std::string(FN__PIXILS_UI_STYLE__MAKE_INSETS));
  } // namespace HostType

  namespace Function
  {
    FUNC(MakeBorder, make);
    FUNC(MakeBorderStyle, make);
    FUNC(MakeStyle, make);
    FUNC(MakeLayout, make);
    FUNC(MakeLayoutGap, make, make_key, make_num);
    FUNC(MakeBackground, make_color, make_image, make_map);
    FUNC(MakeInsets, make_num, make_map, make_vec);

  } // namespace Function

  NATIVE_ADAPTER(StyleAdapter,
                 UI::Style,
                 (background,
                  margin,
                  border,
                  padding,
                  width,
                  height,
                  position,
                  top,
                  left,
                  layout,
                  hidden,
                  hover),
                 (hidden));
  NATIVE_ADAPTER(LayoutAdapter, UI::Style::Layout, (direction, gap));
  NATIVE_ADAPTER(LayoutGapAdapter, UI::Style::Layout::Gap, (mode, size));
  NATIVE_ADAPTER(BackgroundAdapter, UI::Style::Background, (color, image));
  NATIVE_ADAPTER(BorderAdapter, UI::Style::Border, (thickness, line_style, color, trim));
  NATIVE_SUB_ADAPTER(BorderAdapter,
                     (BorderStyleAdapter, UI::Style::BorderStyle),
                     (t, r, b, l));
  NATIVE_ADAPTER(InsetsAdapter, UI::Style::Insets, (t, r, b, l));

  class StyleNamespace : public Lisple::Namespace
  {
   public:
    StyleNamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__STYLE_NAMESPACE_H */
