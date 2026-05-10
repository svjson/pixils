#include "pixils/binding/ui/style/style_adapter.h"

#include <pixils/binding/color_namespace.h>
#include <pixils/binding/ui/style/style_definition.h>
#include <pixils/binding/ui/style/style_host_type.h>

#include <lisple/host/accessor.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  NATIVE_ADAPTER_IMPL(StyleAdapter,
                      UI::Style,
                      &HostType::STYLE,
                      (background),
                      (margin),
                      (border),
                      (padding),
                      (text),
                      (box_sizing),
                      (rw, "width", width),
                      (rw, "height", height),
                      (rw, "position", position),
                      (rw, "top", top),
                      (rw, "left", left),
                      (layout),
                      (rw, "hidden", hidden),
                      (hover),
                      ("focus-within", focus_within),
                      (focus))

  NOBJ_PROP_GET(StyleAdapter, background)
  {
    if (!get_self_object().background) return Lisple::Constant::NIL;
    if (get_self_object().background->color && get_self_object().background->image)
      return BackgroundAdapter::make_ref(*get_self_object().background);
    if (get_self_object().background->color)
      return ColorAdapter::make_ref(*get_self_object().background->color);
    if (get_self_object().background->image)
      return BackgroundAdapter(*get_self_object().background).get_image();
    return Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, margin)
  {
    return get_self_object().margin ? InsetsAdapter::make_ref(*get_self_object().margin)
                                    : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, border)
  {
    auto& style = get_self_object();
    if (!style.border) return Lisple::Constant::NIL;
    return BorderStyleAdapter::make_ref(*style.border);
  }

  NOBJ_PROP_GET(StyleAdapter, padding)
  {
    return get_self_object().padding ? InsetsAdapter::make_ref(*get_self_object().padding)
                                     : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, text)
  {
    return get_self_object().text ? StyleTextAdapter::make_ref(*get_self_object().text)
                                  : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, box_sizing)
  {
    return StyleDefinition::box_sizing_to_value(get_self_object().box_sizing);
  }

  NOBJ_PROP_GET(StyleAdapter, width)
  {
    return StyleDefinition::size_to_value(get_self_object().width);
  }

  NOBJ_PROP_SET(StyleAdapter, width)
  {
    get_self_object().width = StyleDefinition::parse_size(value);
  }

  NOBJ_PROP_GET(StyleAdapter, height)
  {
    return StyleDefinition::size_to_value(get_self_object().height);
  }

  NOBJ_PROP_SET(StyleAdapter, height)
  {
    get_self_object().height = StyleDefinition::parse_size(value);
  }

  NOBJ_PROP_GET(StyleAdapter, position)
  {
    return StyleDefinition::position_mode_to_value(get_self_object().position);
  }

  NOBJ_PROP_SET(StyleAdapter, position)
  {
    get_self_object().position = StyleDefinition::parse_position_mode(value);
  }

  NOBJ_PROP_GET(StyleAdapter, top)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().top);
  }

  NOBJ_PROP_SET(StyleAdapter, top)
  {
    get_self_object().top = StyleDefinition::parse_optional_int(value);
  }

  NOBJ_PROP_GET(StyleAdapter, left)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().left);
  }

  NOBJ_PROP_SET(StyleAdapter, left)
  {
    get_self_object().left = StyleDefinition::parse_optional_int(value);
  }

  NOBJ_PROP_GET(StyleAdapter, layout)
  {
    return get_self_object().layout ? LayoutAdapter::make_ref(*get_self_object().layout)
                                    : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, hidden)
  {
    return StyleDefinition::optional_bool_to_value(get_self_object().hidden);
  }

  NOBJ_PROP_SET(StyleAdapter, hidden)
  {
    get_self_object().hidden = StyleDefinition::parse_optional_bool(value);
  }

  NOBJ_PROP_GET(StyleAdapter, hover)
  {
    return get_self_object().hover ? StyleAdapter::make_ref(*get_self_object().hover)
                                   : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, focus_within)
  {
    return get_self_object().focus_within
             ? StyleAdapter::make_ref(*get_self_object().focus_within)
             : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, focus)
  {
    return get_self_object().focus ? StyleAdapter::make_ref(*get_self_object().focus)
                                   : Lisple::Constant::NIL;
  }

  NATIVE_ADAPTER_IMPL(LayoutAdapter,
                      UI::Style::Layout,
                      &HostType::STYLE_LAYOUT,
                      (direction),
                      ("align-items", align_items),
                      (gap));

  NOBJ_PROP_GET(LayoutAdapter, direction)
  {
    return StyleDefinition::layout_direction_to_value(get_self_object().direction);
  }

  NOBJ_PROP_GET(LayoutAdapter, align_items)
  {
    return StyleDefinition::layout_align_items_to_value(get_self_object().align_items);
  }

  NOBJ_PROP_GET(LayoutAdapter, gap)
  {
    return get_self_object().gap ? LayoutGapAdapter::make_ref(*get_self_object().gap)
                                 : Lisple::Constant::NIL;
  }

  NATIVE_ADAPTER_IMPL(LayoutGapAdapter,
                      UI::Style::Layout::Gap,
                      &HostType::STYLE_LAYOUT_GAP,
                      (mode),
                      (size));

  NOBJ_PROP_GET(LayoutGapAdapter, mode)
  {
    return StyleDefinition::layout_gap_mode_to_value(get_self_object().mode);
  }

  NOBJ_PROP_GET(LayoutGapAdapter, size)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().size);
  }

  NATIVE_ADAPTER_IMPL(StyleTextAdapter,
                      UI::Style::Text,
                      &HostType::STYLE_TEXT,
                      (color),
                      (font),
                      (scale),
                      (align),
                      (wrap));

  NOBJ_PROP_GET(StyleTextAdapter, color)
  {
    return get_self_object().color ? ColorAdapter::make_ref(*get_self_object().color)
                                   : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleTextAdapter, font)
  {
    return get_self_object().font ? Lisple::RTValue::keyword(*get_self_object().font)
                                  : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleTextAdapter, scale)
  {
    return get_self_object().scale ? Lisple::RTValue::number(*get_self_object().scale)
                                   : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleTextAdapter, align)
  {
    return StyleDefinition::text_align_to_value(get_self_object().align);
  }

  NOBJ_PROP_GET(StyleTextAdapter, wrap)
  {
    return StyleDefinition::text_wrap_to_value(get_self_object().wrap);
  }

  NATIVE_ADAPTER_IMPL(ThemeAdapter, UI::Theme, &HostType::THEME, (name));
  NOBJ_PROP_GET__FIELD(ThemeAdapter, name);

  NATIVE_ADAPTER_IMPL(BackgroundAdapter,
                      UI::Style::Background,
                      &HostType::STYLE_BACKGROUND,
                      (color),
                      (image));

  NOBJ_PROP_GET(BackgroundAdapter, image)
  {
    return get_self_object().image
             ? Lisple::RTValue::keyword(get_self_object().image->first + "/" +
                                        get_self_object().image->second)
             : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(BackgroundAdapter, color)
  {
    return get_self_object().color ? ColorAdapter::make_ref(*get_self_object().color)
                                   : Lisple::Constant::NIL;
  }

  NATIVE_ADAPTER_IMPL(BorderAdapter,
                      UI::Style::Border,
                      &HostType::BORDER,
                      (thickness),
                      ("line-style", line_style),
                      (color),
                      (trim));

  NOBJ_PROP_GET(BorderAdapter, thickness)
  {
    return get_self_object().thickness
             ? Lisple::RTValue::number(*get_self_object().thickness)
             : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(BorderAdapter, line_style)
  {
    return StyleDefinition::line_style_to_value(get_self_object().line_style);
  }

  NOBJ_PROP_GET_OPT_ADAPTER__FIELD(BorderAdapter, color, ColorAdapter);

  NOBJ_PROP_GET(BorderAdapter, trim)
  {
    return StyleDefinition::trim_to_value(get_self_object().trim);
  }

  NATIVE_SUB_ADAPTER_IMPL(BorderAdapter,
                          UI::Style::Border,
                          (BorderStyleAdapter, UI::Style::BorderStyle),
                          &HostType::BORDER_STYLE,
                          ("top", t),
                          ("right", r),
                          ("bottom", b),
                          ("left", l))

  NOBJ_PROP_GET_OPT_ADAPTER__FIELD(BorderStyleAdapter, t, BorderAdapter);
  NOBJ_PROP_GET_OPT_ADAPTER__FIELD(BorderStyleAdapter, r, BorderAdapter);
  NOBJ_PROP_GET_OPT_ADAPTER__FIELD(BorderStyleAdapter, b, BorderAdapter);
  NOBJ_PROP_GET_OPT_ADAPTER__FIELD(BorderStyleAdapter, l, BorderAdapter);

  NATIVE_ADAPTER_IMPL(InsetsAdapter,
                      UI::Style::Insets,
                      &HostType::STYLE_INSETS,
                      ("top", t),
                      ("right", r),
                      ("bottom", b),
                      ("left", l));

  NOBJ_PROP_GET__FIELD(InsetsAdapter, t);
  NOBJ_PROP_GET__FIELD(InsetsAdapter, r);
  NOBJ_PROP_GET__FIELD(InsetsAdapter, b);
  NOBJ_PROP_GET__FIELD(InsetsAdapter, l);
} // namespace Pixils::Script
