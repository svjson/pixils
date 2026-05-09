#ifndef PIXILS__UI__STYLE__ADAPTER_H
#define PIXILS__UI__STYLE__ADAPTER_H

#include <pixils/ui/style.h>
#include <pixils/ui/theme.h>

#include <lisple/host.h>
#include <lisple/host/object.h>

namespace Pixils::Script
{
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
} // namespace Pixils::Script

#endif /* PIXILS__UI__STYLE__ADAPTER_H */
