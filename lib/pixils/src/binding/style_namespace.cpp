
#include "pixils/binding/style_namespace.h"

#include "pixils/binding/color_namespace.h"
#include "pixils/ui/style.h"

#include <lisple/host/accessor.h>
#include <lisple/host/object.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>
#include <lisple/type.h>
namespace Pixils::Script
{

  namespace Function
  {
    static std::optional<UI::Style::Trim> parse_trim(const Lisple::sptr_rtval& value)
    {
      if (*value == *Lisple::Constant::NIL) return std::nullopt;

      switch (value->type)
      {
      case Lisple::RTValue::Type::NUMBER:
        return UI::Style::Trim{value->num().get_int()};
      case Lisple::RTValue::Type::VECTOR:
        switch (Lisple::count(*value))
        {
        case 1:
          return UI::Style::Trim{Lisple::get_child(*value, 0)->num().get_int()};
        case 2:
          return UI::Style::Trim{Lisple::get_child(*value, 0)->num().get_int(),
                                 Lisple::get_child(*value, 1)->num().get_int()};
        default:
          return std::nullopt;
        }
      default:
        return std::nullopt;
      }
    }

    /** MakeStyle - make-style */
    FUNC_IMPL(MakeStyle,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeStyle::exec_make))));

    FUNC_IMPL(MakeLayout,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeLayout::exec_make))));

    EXEC_BODY(MakeLayout, exec_make)
    {
      static Lisple::MapSchema layout_schema({}, {{"direction", &Lisple::Type::KEY}});

      auto layout = std::make_unique<UI::Style::Layout>();
      auto opts = layout_schema.bind(ctx, *args[0]);

      if (opts.contains("direction"))
      {
        auto dir_str = opts.str("direction");
        if (dir_str == "row")
        {
          layout->direction = UI::LayoutDirection::ROW;
        }
        else
        {
          layout->direction = UI::LayoutDirection::COLUMN;
        }
      }

      return LayoutAdapter::claim(std::move(layout));
    }

    EXEC_BODY(MakeStyle, exec_make)
    {
      static Lisple::MapSchema style_schema({},
                                            {{"background", &HostType::STYLE_BACKGROUND},
                                             {"margin", &HostType::STYLE_INSETS},
                                             {"border", &HostType::BORDER_STYLE},
                                             {"padding", &HostType::STYLE_INSETS},
                                             {"layout", &HostType::STYLE_LAYOUT},
                                             {"width", &Lisple::Type::NUMBER},
                                             {"height", &Lisple::Type::NUMBER},
                                             {"position", &Lisple::Type::KEY},
                                             {"top", &Lisple::Type::NUMBER},
                                             {"left", &Lisple::Type::NUMBER},
                                             {"hidden", &Lisple::Type::ANY},
                                             {"hover", &HostType::STYLE}});

      auto style = std::make_unique<UI::Style>();
      auto opts = style_schema.bind(ctx, *args[0]);

      style->background = opts.optional_obj<UI::Style::Background>("background");
      style->margin = opts.optional_obj<UI::Style::Insets>("margin");
      style->padding = opts.optional_obj<UI::Style::Insets>("padding");
      style->border = opts.optional_obj<UI::Style::BorderStyle>("border");
      style->layout = opts.optional_obj<UI::Style::Layout>("layout");

      if (opts.contains("width")) style->width = opts.i32("width");
      if (opts.contains("height")) style->height = opts.i32("height");
      if (opts.contains("top")) style->top = opts.i32("top");
      if (opts.contains("left")) style->left = opts.i32("left");

      if (opts.contains("position"))
      {
        auto pos_str = opts.str("position");
        if (pos_str == "absolute")
        {
          style->position = UI::PositionMode::ABSOLUTE;
        }
        else
        {
          style->position = UI::PositionMode::FLOW;
        }
      }

      if (opts.contains("hidden")) style->hidden = Lisple::is_truthy(*opts.val("hidden"));

      auto hover_style = opts.optional_obj<UI::Style>("hover");
      if (hover_style) style->hover = std::make_unique<UI::Style>(*hover_style);

      return StyleAdapter::claim(std::move(style));
    }

    /** MakeBackground - make-background */
    FUNC_IMPL(MakeBackground,
              MULTI_SIG((FN_ARGS((&HostType::COLOR)),
                         EXEC_DISPATCH(&MakeBackground::exec_make_color)),
                        (FN_ARGS((&Lisple::Type::KEY)),
                         EXEC_DISPATCH(&MakeBackground::exec_make_image)),
                        (FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakeBackground::exec_make_map))));

    EXEC_BODY(MakeBackground, exec_make_color)
    {
      return BackgroundAdapter::make_unique(Lisple::obj<Color>(*args[0]));
    }

    EXEC_BODY(MakeBackground, exec_make_image)
    {
      return BackgroundAdapter::make_unique(args[0]->qual());
    }
    EXEC_BODY(MakeBackground, exec_make_map)
    {
      if (Lisple::Dict::contains_key(*args[0], "r"))
      {
        auto cres = HostType::COLOR.coerce(ctx, args[0]);
        if (cres.success)
        {
          Lisple::sptr_rtval_v cargs = {cres.result};
          return exec_make_color(ctx, cargs);
        }
      }
      static Lisple::MapSchema background_schema(
        {},
        {{"color", &HostType::COLOR}, {"image", &Lisple::Type::KEY}});

      auto bg = std::make_unique<UI::Style::Background>();

      auto opts = background_schema.bind(ctx, *args[0]);
      bg->color = opts.optional_obj<Color>("color");

      auto img_key = opts.val("image");
      if (img_key->type != Lisple::RTValue::Type::NIL)
      {
        bg->image = img_key->qual();
      }

      return BackgroundAdapter::claim(std::move(bg));
    }

    /** Border/BorderStyle common props */
    static void apply_border_props(UI::Style::Border& border,
                                   Lisple::MapSchema::Inspector& opts)
    {
      if (auto thickness = opts.i32("thickness", -1); thickness > 0)
      {
        border.thickness = thickness;
      }
      if (auto line_style = opts.val("line-style"); *line_style != *Lisple::Constant::NIL)
      {
        auto lstyle = line_style->str();
        if (lstyle == "solid")
        {
          border.line_style = UI::Style::LineStyle::SOLID;
        }
        else if (lstyle == "bevel")
        {
          border.line_style = UI::Style::LineStyle::BEVEL;
        }
      }
      border.color = opts.optional_obj<Color>("color");
      border.trim = parse_trim(opts.val("trim"));
    }

    /** MakeBorder - make-border */
    FUNC_IMPL(MakeBorder,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeBorder::exec_make))));

    EXEC_BODY(MakeBorder, exec_make)
    {
      static Lisple::MapSchema border_schema({},
                                             {{"thickness", &Lisple::Type::NUMBER},
                                              {"line-style", &Lisple::Type::KEY},
                                              {"color", &HostType::COLOR},
                                              {"trim", &Lisple::Type::ANY}});

      auto opts = border_schema.bind(ctx, *args[0]);

      std::unique_ptr<UI::Style::Border> border = std::make_unique<UI::Style::Border>();
      apply_border_props(*border, opts);

      return BorderAdapter::claim(std::move(border));
    }

    /** MakeBorderStyle - make-border */
    FUNC_IMPL(MakeBorderStyle,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeBorderStyle::exec_make))));

    EXEC_BODY(MakeBorderStyle, exec_make)
    {
      static Lisple::MapSchema border_style_schema({},
                                                   {{"thickness", &Lisple::Type::NUMBER},
                                                    {"line-style", &Lisple::Type::KEY},
                                                    {"color", &HostType::COLOR},
                                                    {"trim", &Lisple::Type::ANY},
                                                    {"top", &HostType::BORDER},
                                                    {"right", &HostType::BORDER},
                                                    {"bottom", &HostType::BORDER},
                                                    {"left", &HostType::BORDER}});

      auto opts = border_style_schema.bind(ctx, *args[0]);

      std::unique_ptr<UI::Style::BorderStyle> border =
        std::make_unique<UI::Style::BorderStyle>();
      apply_border_props(*border, opts);

      border->t = opts.optional_obj<UI::Style::Border>("top");
      border->r = opts.optional_obj<UI::Style::Border>("right");
      border->b = opts.optional_obj<UI::Style::Border>("bottom");
      border->l = opts.optional_obj<UI::Style::Border>("left");

      return BorderStyleAdapter::claim(std::move(border));
    }

    /** MakeInsets - make-insets */
    FUNC_IMPL(MakeInsets,
              MULTI_SIG((FN_ARGS((&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MakeInsets::exec_make_num)),
                        (FN_ARGS((&Lisple::Type::ARRAY_OF_NUMBER)),
                         EXEC_DISPATCH(&MakeInsets::exec_make_vec)),
                        (FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakeInsets::exec_make_map))));

    EXEC_BODY(MakeInsets, exec_make_map)
    {
      static Lisple::MapSchema padding_map_schema({},
                                                  {{"t", &Lisple::Type::NUMBER},
                                                   {"r", &Lisple::Type::NUMBER},
                                                   {"b", &Lisple::Type::NUMBER},
                                                   {"l", &Lisple::Type::NUMBER}});

      auto opts = padding_map_schema.bind(ctx, *args[0]);

      std::unique_ptr<UI::Style::Insets> padding = std::make_unique<UI::Style::Insets>();
      padding->t = opts.i32("t", 0);
      padding->r = opts.i32("r", 0);
      padding->b = opts.i32("b", 0);
      padding->l = opts.i32("l", 0);

      return InsetsAdapter::claim(std::move(padding));
    }

    EXEC_BODY(MakeInsets, exec_make_num)
    {
      int p = args.front()->num().get_int();

      std::unique_ptr<UI::Style::Insets> padding = std::make_unique<UI::Style::Insets>();
      padding->t = p;
      padding->r = p;
      padding->b = p;
      padding->l = p;

      return InsetsAdapter::claim(std::move(padding));
    }

    EXEC_BODY(MakeInsets, exec_make_vec)
    {
      int t = 0, r = 0, b = 0, l = 0;

      switch (Lisple::count(*args[0]))
      {
      case 1:
        t = r = b = l = Lisple::get_child(*args[0], 0)->num().get_int();
        break;
      case 2:
        t = b = Lisple::get_child(*args[0], 0)->num().get_int();
        r = l = Lisple::get_child(*args[0], 1)->num().get_int();
        break;
      case 4:
        t = Lisple::get_child(*args[0], 0)->num().get_int();
        r = Lisple::get_child(*args[0], 1)->num().get_int();
        b = Lisple::get_child(*args[0], 2)->num().get_int();
        l = Lisple::get_child(*args[0], 3)->num().get_int();
        break;
      default:
        return Lisple::Constant::NIL;
      };

      return InsetsAdapter::make_unique(t, r, b, l);
    }

  } // namespace Function

  NATIVE_ADAPTER_IMPL(StyleAdapter,
                      UI::Style,
                      &HostType::STYLE,
                      (background),
                      (margin),
                      (border),
                      (padding),
                      (width),
                      (height),
                      (position),
                      (top),
                      (left),
                      (layout),
                      (rw, "hidden", hidden),
                      (hover))

  NATIVE_ADAPTER_IMPL(LayoutAdapter,
                      UI::Style::Layout,
                      &HostType::STYLE_LAYOUT,
                      (direction));

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
    if (!style.border)
    {
      return Lisple::Constant::NIL;
    }

    return BorderStyleAdapter::make_ref(*style.border);
  }

  NOBJ_PROP_GET(StyleAdapter, padding)
  {
    return get_self_object().padding ? InsetsAdapter::make_ref(*get_self_object().padding)
                                     : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, width)
  {
    return get_self_object().width ? Lisple::RTValue::number(*get_self_object().width)
                                   : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, height)
  {
    return get_self_object().height ? Lisple::RTValue::number(*get_self_object().height)
                                    : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, position)
  {
    if (!get_self_object().position) return Lisple::Constant::NIL;
    return Lisple::RTValue::keyword(
      *get_self_object().position == UI::PositionMode::ABSOLUTE ? "absolute" : "flow");
  }

  NOBJ_PROP_GET(StyleAdapter, top)
  {
    return get_self_object().top ? Lisple::RTValue::number(*get_self_object().top)
                                 : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, left)
  {
    return get_self_object().left ? Lisple::RTValue::number(*get_self_object().left)
                                  : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, layout)
  {
    return get_self_object().layout ? LayoutAdapter::make_ref(*get_self_object().layout)
                                    : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(LayoutAdapter, direction)
  {
    if (!get_self_object().direction) return Lisple::Constant::NIL;
    return Lisple::RTValue::keyword(
      *get_self_object().direction == UI::LayoutDirection::ROW ? "row" : "column");
  }

  NOBJ_PROP_GET(StyleAdapter, hidden)
  {
    auto& h = get_self_object().hidden;
    if (!h) return Lisple::Constant::NIL;
    return *h ? Lisple::Constant::BOOL_TRUE : Lisple::Constant::BOOL_FALSE;
  }

  NOBJ_PROP_SET(StyleAdapter, hidden)
  {
    if (value->type != Lisple::RTValue::Type::BOOL)
    {
      get_self_object().hidden.reset();
    }
    else
    {
      get_self_object().hidden = Lisple::is_truthy(*value);
    }
  }

  NOBJ_PROP_GET(StyleAdapter, hover)
  {
    return get_self_object().hover ? StyleAdapter::make_ref(*get_self_object().hover)
                                   : Lisple::Constant::NIL;
  }

  /** BackgroundAdapter */
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
    return get_self_object().color ? ColorAdapter ::make_ref(*get_self_object().color)
                                   : Lisple::Constant::NIL;
  };

  /** BorderAdapter */
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
    if (!get_self_object().line_style) return Lisple::Constant::NIL;
    switch (*get_self_object().line_style)
    {
    case UI::Style::LineStyle::SOLID:
      return Lisple::RTValue::keyword("solid");
    case UI::Style::LineStyle::BEVEL:
      return Lisple::RTValue::keyword("bevel");
    }
    return Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET_OPT_ADAPTER__FIELD(BorderAdapter, color, ColorAdapter);

  NOBJ_PROP_GET(BorderAdapter, trim)
  {
    if (!get_self_object().trim) return Lisple::Constant::NIL;

    return Lisple::RTValue::vector({Lisple::RTValue::number(get_self_object().trim->start),
                                    Lisple::RTValue::number(get_self_object().trim->end)});
  }

  /** BorderStyleAdapter */
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

  /** InsetsAdapter */
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

  StyleNamespace::StyleNamespace()
    : Lisple::Namespace("pixils.ui.style")
  {
    values.emplace("make-border", Function::MakeBorder::make());
    values.emplace("make-border-style", Function::MakeBorderStyle::make());
    values.emplace("make-style", Function::MakeStyle::make());
    values.emplace("make-layout", Function::MakeLayout::make());
    values.emplace("make-background", Function::MakeBackground::make());
    values.emplace("make-insets", Function::MakeInsets::make());
  }

} // namespace Pixils::Script
