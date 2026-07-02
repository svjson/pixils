#ifndef PIXILS__UI__STYLE__ADAPTER_H
#define PIXILS__UI__STYLE__ADAPTER_H

#include <pixils/ui/style.h>
#include <pixils/ui/theme.h>

#include <roo/host/object.h>

namespace Pixils::Script
{
  NATIVE_ADAPTER(
    StyleAdapter,
    UI::Style,
    (background,
     image,
     margin,
     border,
     padding,
     corner_radius,
     text,
     box_sizing,
     opacity,
     scale,
     width,
     height,
     min_width,
     min_height,
     max_width,
     max_height,
     position,
     top,
     left,
     layout,
     visibility,
     hit_test,
     clip,
     cursor,
     hover,
     focus_within,
     focus),
    (clip,
     cursor,
     height,
     hit_test,
     left,
     max_height,
     max_width,
     min_height,
     min_width,
     opacity,
     position,
     scale,
     top,
     visibility,
     width));
  NATIVE_ADAPTER(LayoutAdapter,
                 UI::Style::Layout,
                 (direction, align_items, gap, wrap, line_gap));
  NATIVE_ADAPTER(LayoutGapAdapter, UI::Style::Layout::Gap, (mode, size));
  NATIVE_ADAPTER(StyleTextAdapter, UI::Style::Text, (color, font, scale, align, wrap));
  NATIVE_ADAPTER(ThemeAdapter, UI::Theme, (name));
  NATIVE_ADAPTER(BackgroundAdapter,
                 UI::Style::Background,
                 (color, image, source, fit, align, offset, opacity, repeat_x, repeat_y));
  NATIVE_ADAPTER(BorderAdapter, UI::Style::Border, (thickness, line_style, color, trim));
  NATIVE_SUB_ADAPTER(BorderAdapter,
                     (BorderStyleAdapter, UI::Style::BorderStyle),
                     (t, r, b, l));
  NATIVE_ADAPTER(InsetsAdapter, UI::Style::Insets, (t, r, b, l));
} // namespace Pixils::Script

#endif /* PIXILS__UI__STYLE__ADAPTER_H */
