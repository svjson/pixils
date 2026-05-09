#ifndef PIXILS__UI__STYLE__CONSTANT_H
#define PIXILS__UI__STYLE__CONSTANT_H

#include <string_view>

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
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_TEXT =
    std::string_view("pixils.ui.style/make-text");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_INSETS =
    std::string_view("pixils.ui.style/make-insets");
  inline constexpr std::string_view FN__PIXILS_UI_STYLE__MAKE_BACKGROUND =
    std::string_view("pixils.ui.style/make-background");

} // namespace Pixils::Script

#endif
