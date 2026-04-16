
#include "pixils/binding/style_namespace.h"

#include "pixils/binding/color_namespace.h"
#include "pixils/ui/style.h"

#include <iostream>
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
    /** MakeStyle - make-style */
    FUNC_IMPL(MakeStyle,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeStyle::exec_make))));

    EXEC_BODY(MakeStyle, exec_make)
    {
      static Lisple::MapSchema style_schema({},
                                            {{"background", &HostType::STYLE_BACKGROUND},
                                             {"padding", &HostType::STYLE_PADDING},
                                             {"hover", &HostType::STYLE}});

      auto style = std::make_unique<UI::Style>();
      auto opts = style_schema.bind(ctx, *args[0]);

      style->background = opts.optional_obj<UI::Style::Background>("background");
      style->padding = opts.optional_obj<UI::Style::Padding>("padding");
      auto hover_style = opts.optional_obj<UI::Style>("hover");
      if (hover_style)
      {
        style->hover_style = std::make_unique<UI::Style>(*hover_style);
      }

      std::cout << "Style created with bg? " << style->background.has_value() << std::endl;
      std::cout << "Style created with bg-color? "
                << (style->background && style->background->color) << std::endl;

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
      std::cout << "MakeBackground::make_color" << std::endl;
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

    /** MakePadding - make-padding */
    FUNC_IMPL(MakePadding,
              MULTI_SIG((FN_ARGS((&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MakePadding::exec_make_num)),
                        (FN_ARGS((&Lisple::Type::ARRAY_OF_NUMBER)),
                         EXEC_DISPATCH(&MakePadding::exec_make_vec)),
                        (FN_ARGS((&Lisple::Type::ARRAY)),
                         EXEC_DISPATCH(&MakePadding::exec_make_map))));

    EXEC_BODY(MakePadding, exec_make_map)
    {
      static Lisple::MapSchema padding_map_schema({},
                                                  {{"t", &Lisple::Type::NUMBER},
                                                   {"r", &Lisple::Type::NUMBER},
                                                   {"b", &Lisple::Type::NUMBER},
                                                   {"l", &Lisple::Type::NUMBER}});

      auto opts = padding_map_schema.bind(ctx, *args[0]);

      std::unique_ptr<UI::Style::Padding> padding = std::make_unique<UI::Style::Padding>();
      padding->t = opts.i32("t", 0);
      padding->r = opts.i32("r", 0);
      padding->b = opts.i32("b", 0);
      padding->l = opts.i32("l", 0);

      return PaddingAdapter::claim(std::move(padding));
    }

    EXEC_BODY(MakePadding, exec_make_num)
    {
      int p = args.front()->num().get_int();

      std::unique_ptr<UI::Style::Padding> padding = std::make_unique<UI::Style::Padding>();
      padding->t = p;
      padding->r = p;
      padding->b = p;
      padding->l = p;

      return PaddingAdapter::claim(std::move(padding));
    }

    EXEC_BODY(MakePadding, exec_make_vec)
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
        r = Lisple::get_child(*args[1], 0)->num().get_int();
        b = Lisple::get_child(*args[2], 0)->num().get_int();
        l = Lisple::get_child(*args[3], 0)->num().get_int();
        break;
      default:
        return Lisple::Constant::NIL;
      };

      return PaddingAdapter::make_unique(t, r, b, l);
    }

  } // namespace Function

  NATIVE_ADAPTER_IMPL(StyleAdapter,
                      UI::Style,
                      &HostType::STYLE,
                      (background),
                      (padding),
                      (hover_style))

  NOBJ_PROP_GET(StyleAdapter, background)
  {
    if (!get_self_object().background) return Lisple::Constant::NIL;
    if (get_self_object().background->color && get_self_object().background->image)
    {
      return BackgroundAdapter::make_ref(*get_self_object().background);
    }
    if (get_self_object().background->color)
    {
      return ColorAdapter::make_ref(*get_self_object().background->color);
    }
    if (get_self_object().background->image)
    {
      return BackgroundAdapter(*get_self_object().background).get_image();
    }

    return Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, padding)
  {
    return get_self_object().padding ? PaddingAdapter::make_ref(*get_self_object().padding)
                                     : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(StyleAdapter, hover_style)
  {
    return get_self_object().hover_style
             ? StyleAdapter::make_ref(*get_self_object().hover_style)
             : Lisple::Constant::NIL;
  }

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

  NATIVE_ADAPTER_IMPL(PaddingAdapter,
                      UI::Style::Padding,
                      &HostType::STYLE_PADDING,
                      (t),
                      (r),
                      (b),
                      (l));

  NOBJ_PROP_GET__FIELD(PaddingAdapter, t);
  NOBJ_PROP_GET__FIELD(PaddingAdapter, r);
  NOBJ_PROP_GET__FIELD(PaddingAdapter, b);
  NOBJ_PROP_GET__FIELD(PaddingAdapter, l);

  StyleNamespace::StyleNamespace()
    : Lisple::Namespace("pixils.ui.style")
  {
    values.emplace("make-style", Function::MakeStyle::make());
    values.emplace("make-background", Function::MakeBackground::make());
    values.emplace("make-padding", Function::MakePadding::make());
  }

} // namespace Pixils::Script
