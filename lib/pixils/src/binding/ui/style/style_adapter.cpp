#include "pixils/binding/ui/style/style_adapter.h"

#include <pixils/binding/color_namespace.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/ui/style/style_definition.h>
#include <pixils/binding/ui/style/style_host_type.h>

#include <algorithm>
#include <roo/host/accessor.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace
  {
    Roo::sptr_val background_fit_to_value(
      const std::optional<UI::Style::Background::Fit>& fit)
    {
      if (!fit) return Roo::Constant::NIL;
      switch (*fit)
      {
      case UI::Style::Background::Fit::NONE:
        return Roo::keyword("none");
      case UI::Style::Background::Fit::CONTAIN:
        return Roo::keyword("contain");
      case UI::Style::Background::Fit::COVER:
        return Roo::keyword("cover");
      case UI::Style::Background::Fit::FILL:
        return Roo::keyword("fill");
      }
      return Roo::Constant::NIL;
    }

    Roo::sptr_val background_align_to_value(
      const std::optional<UI::Style::Background::Align>& align_x,
      const std::optional<UI::Style::Background::Align>& align_y)
    {
      if (!align_x && !align_y) return Roo::Constant::NIL;

      auto align_name = [](UI::Style::Background::Align align)
      {
        switch (align)
        {
        case UI::Style::Background::Align::START:
          return "start";
        case UI::Style::Background::Align::CENTER:
          return "center";
        case UI::Style::Background::Align::END:
          return "end";
        }
        return "start";
      };

      return Roo::map(
        {Roo::keyword("x"),
         Roo::keyword(align_name(align_x.value_or(UI::Style::Background::Align::START))),
         Roo::keyword("y"),
         Roo::keyword(
           align_name(align_y.value_or(UI::Style::Background::Align::START)))});
    }

  } // namespace

  NATIVE_ADAPTER_IMPL(StyleAdapter,
                      UI::Style,
                      &HostType::STYLE,
                      (background),
                      (margin),
                      (border),
                      (padding),
                      ("corner-radius", corner_radius),
                      (text),
                      (box_sizing),
                      (rw, "opacity", opacity),
                      (rw, "scale", scale),
                      (rw, "width", width),
                      (rw, "height", height),
                      (rw, "min-width", min_width),
                      (rw, "min-height", min_height),
                      (rw, "max-width", max_width),
                      (rw, "max-height", max_height),
                      (rw, "position", position),
                      (rw, "top", top),
                      (rw, "left", left),
                      (layout),
                      (rw, "visibility", visibility),
                      (rw, "hit-test", hit_test),
                      (rw, "clip", clip),
                      (rw, "cursor", cursor),
                      (hover),
                      ("focus-within", focus_within),
                      (focus))

  NOBJ_PROP_GET(StyleAdapter, background)
  {
    if (!get_self_object().background)
    {
      return Roo::Constant::NIL;
    }

    const auto& background = *get_self_object().background;
    const bool needs_background_adapter =
      (background.color && background.image) || background.source || background.fit ||
      background.align_x || background.align_y || background.offset || background.opacity;

    if (needs_background_adapter)
    {
      return BackgroundAdapter::make_ref(background);
    }
    if (background.color)
    {
      return ColorAdapter::make_ref(*background.color);
    }
    if (background.image)
    {
      return Roo::keyword(background.image->first + "/" + background.image->second);
    }
    return Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, margin)
  {
    return get_self_object().margin ? InsetsAdapter::make_ref(*get_self_object().margin)
                                    : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, border)
  {
    auto& style = get_self_object();
    if (!style.border) return Roo::Constant::NIL;
    return BorderStyleAdapter::make_ref(*style.border);
  }

  NOBJ_PROP_GET(StyleAdapter, padding)
  {
    return get_self_object().padding ? InsetsAdapter::make_ref(*get_self_object().padding)
                                     : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, corner_radius)
  {
    return StyleDefinition::corner_radius_to_value(get_self_object().corner_radius);
  }

  NOBJ_PROP_GET(StyleAdapter, text)
  {
    return get_self_object().text ? StyleTextAdapter::make_ref(*get_self_object().text)
                                  : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, box_sizing)
  {
    return StyleDefinition::box_sizing_to_value(get_self_object().box_sizing);
  }

  NOBJ_PROP_GET(StyleAdapter, scale)
  {
    return StyleDefinition::scale_to_value(get_self_object().scale);
  }

  NOBJ_PROP_GET(StyleAdapter, opacity)
  {
    return get_self_object().opacity ? Roo::number(*get_self_object().opacity)
                                     : Roo::Constant::NIL;
  }

  NOBJ_PROP_SET(StyleAdapter, opacity)
  {
    if (!value || value->type == Roo::Value::Type::NIL)
    {
      get_self_object().opacity = std::nullopt;
      return;
    }
    if (value->type != Roo::Value::Type::NUMBER)
      throw Roo::TypeError("Style :opacity must be a number");
    get_self_object().opacity = std::clamp(value->f32(), 0.0f, 1.0f);
  }

  NOBJ_PROP_SET(StyleAdapter, scale)
  {
    get_self_object().scale = StyleDefinition::parse_scale(value);
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

  NOBJ_PROP_GET(StyleAdapter, min_width)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().min_width);
  }

  NOBJ_PROP_SET(StyleAdapter, min_width)
  {
    get_self_object().min_width = StyleDefinition::parse_optional_int(value);
  }

  NOBJ_PROP_GET(StyleAdapter, min_height)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().min_height);
  }

  NOBJ_PROP_SET(StyleAdapter, min_height)
  {
    get_self_object().min_height = StyleDefinition::parse_optional_int(value);
  }

  NOBJ_PROP_GET(StyleAdapter, max_width)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().max_width);
  }

  NOBJ_PROP_SET(StyleAdapter, max_width)
  {
    get_self_object().max_width = StyleDefinition::parse_optional_int(value);
  }

  NOBJ_PROP_GET(StyleAdapter, max_height)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().max_height);
  }

  NOBJ_PROP_SET(StyleAdapter, max_height)
  {
    get_self_object().max_height = StyleDefinition::parse_optional_int(value);
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
                                    : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, visibility)
  {
    return StyleDefinition::visibility_to_value(get_self_object().visibility);
  }

  NOBJ_PROP_SET(StyleAdapter, visibility)
  {
    get_self_object().visibility = StyleDefinition::parse_visibility(value);
  }

  NOBJ_PROP_GET(StyleAdapter, hit_test)
  {
    return StyleDefinition::optional_bool_to_value(get_self_object().hit_test);
  }

  NOBJ_PROP_SET(StyleAdapter, hit_test)
  {
    get_self_object().hit_test = StyleDefinition::parse_optional_bool(value);
  }

  NOBJ_PROP_GET(StyleAdapter, clip)
  {
    return StyleDefinition::optional_bool_to_value(get_self_object().clip);
  }

  NOBJ_PROP_SET(StyleAdapter, clip)
  {
    get_self_object().clip = StyleDefinition::parse_optional_bool(value);
  }

  NOBJ_PROP_GET(StyleAdapter, cursor)
  {
    return StyleDefinition::cursor_to_value(get_self_object().cursor);
  }

  NOBJ_PROP_SET(StyleAdapter, cursor)
  {
    get_self_object().cursor = StyleDefinition::parse_cursor(*ctx, value);
  }

  NOBJ_PROP_GET(StyleAdapter, hover)
  {
    return get_self_object().hover ? StyleAdapter::make_ref(*get_self_object().hover)
                                   : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, focus_within)
  {
    return get_self_object().focus_within
             ? StyleAdapter::make_ref(*get_self_object().focus_within)
             : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, focus)
  {
    return get_self_object().focus ? StyleAdapter::make_ref(*get_self_object().focus)
                                   : Roo::Constant::NIL;
  }

  NATIVE_ADAPTER_IMPL(LayoutAdapter,
                      UI::Style::Layout,
                      &HostType::STYLE_LAYOUT,
                      (direction),
                      ("align-items", align_items),
                      (gap),
                      (wrap),
                      ("line-gap", line_gap));

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
                                 : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(LayoutAdapter, wrap)
  {
    return StyleDefinition::layout_wrap_to_value(get_self_object().wrap);
  }

  NOBJ_PROP_GET(LayoutAdapter, line_gap)
  {
    return StyleDefinition::optional_int_to_value(get_self_object().line_gap);
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
    return StyleDefinition::text_color_to_value(get_self_object());
  }

  NOBJ_PROP_GET(StyleTextAdapter, font)
  {
    return get_self_object().font ? Roo::keyword(*get_self_object().font)
                                  : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleTextAdapter, scale)
  {
    return get_self_object().scale ? Roo::number(*get_self_object().scale)
                                   : Roo::Constant::NIL;
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
                      (image),
                      (source),
                      (fit),
                      (align),
                      (offset),
                      (opacity));

  NOBJ_PROP_GET(BackgroundAdapter, image)
  {
    return get_self_object().image ? Roo::keyword(get_self_object().image->first + "/" +
                                                     get_self_object().image->second)
                                   : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(BackgroundAdapter, color)
  {
    return get_self_object().color ? ColorAdapter::make_ref(*get_self_object().color)
                                   : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(BackgroundAdapter, source)
  {
    return get_self_object().source ? RectAdapter::make_ref(*get_self_object().source)
                                    : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(BackgroundAdapter, fit)
  {
    return background_fit_to_value(get_self_object().fit);
  }

  NOBJ_PROP_GET(BackgroundAdapter, align)
  {
    return background_align_to_value(get_self_object().align_x, get_self_object().align_y);
  }

  NOBJ_PROP_GET(BackgroundAdapter, offset)
  {
    return get_self_object().offset ? PointAdapter::make_ref(*get_self_object().offset)
                                    : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(BackgroundAdapter, opacity)
  {
    return get_self_object().opacity ? Roo::number(*get_self_object().opacity)
                                     : Roo::Constant::NIL;
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
    return get_self_object().thickness ? Roo::number(*get_self_object().thickness)
                                       : Roo::Constant::NIL;
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
