#ifndef PIXILS__UI__STYLE__HOST_TYPE_H
#define PIXILS__UI__STYLE__HOST_TYPE_H

#include <pixils/binding/ui/style/style_constant.h>

#include <lisple/host.h>

namespace Pixils::Script::HostType
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
  HOST_TYPE(STYLE_TEXT, "HStyleText", std::string(FN__PIXILS_UI_STYLE__MAKE_TEXT));
  HOST_TYPE(STYLE_BACKGROUND,
            "HStyleBackground",
            std::string(FN__PIXILS_UI_STYLE__MAKE_BACKGROUND));
  HOST_TYPE(STYLE_INSETS, "HStyleInsets", std::string(FN__PIXILS_UI_STYLE__MAKE_INSETS));
  HOST_TYPE(THEME, "HTheme");

} // namespace Pixils::Script::HostType

#endif /* PIXILS__UI__STYLE__HOST_TYPE_H */
