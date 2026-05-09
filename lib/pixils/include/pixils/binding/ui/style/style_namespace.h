
#ifndef PIXILS__STYLE_NAMESPACE_H
#define PIXILS__STYLE_NAMESPACE_H

#include <pixils/ui/style.h>
#include <pixils/ui/theme.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  namespace Function
  {
    FUNC(MakeBorder, make);
    FUNC(MakeBorderStyle, make);
    FUNC(MakeStyle, make);
    FUNC(MakeLayout, make);
    FUNC(MakeLayoutGap, make, make_key, make_num);
    FUNC(MakeText, make);
    FUNC(MakeBackground, make_color, make_image, make_map);
    FUNC(MakeInsets, make_num, make_map, make_vec);

  } // namespace Function

  NATIVE_ADAPTER(StyleAdapter,
                 UI::Style,
                 (background,
                  margin,
                  border,
                  padding,
                  text,
                  box_sizing,
                  width,
                  height,
                  position,
                  top,
                  left,
                  layout,
                  hidden,
                  hover),
                 (height, hidden, left, position, top, width));
  NATIVE_ADAPTER(LayoutAdapter, UI::Style::Layout, (direction, gap));
  NATIVE_ADAPTER(LayoutGapAdapter, UI::Style::Layout::Gap, (mode, size));
  NATIVE_ADAPTER(StyleTextAdapter, UI::Style::Text, (color, font, scale));
  NATIVE_ADAPTER(ThemeAdapter, UI::Theme, (name));
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
