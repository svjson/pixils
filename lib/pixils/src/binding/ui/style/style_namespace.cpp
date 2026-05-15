#include "pixils/binding/ui/style/style_namespace.h"

#include <pixils/binding/color_namespace.h>
#include <pixils/binding/ui/style/style_adapter.h>
#include <pixils/binding/ui/style/style_constant.h>
#include <pixils/binding/ui/style/style_definition.h>
#include <pixils/binding/ui/style/style_host_type.h>

#include <lisple/exec.h>
#include <lisple/runtime/value.h>

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

    FUNC_IMPL(MakeStyle,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeStyle::exec_make))));

    FUNC_IMPL(MakeText,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeText::exec_make))));

    FUNC_IMPL(MakeLayout,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeLayout::exec_make))));

    FUNC_IMPL(MakeLayoutGap,
              MULTI_SIG((FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakeLayoutGap::exec_make)),
                        (FN_ARGS((&Lisple::Type::KEYWORD)),
                         EXEC_DISPATCH(&MakeLayoutGap::exec_make_key)),
                        (FN_ARGS((&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MakeLayoutGap::exec_make_num))));

    EXEC_BODY(MakeLayoutGap, exec_make)
    {
      auto gap = StyleDefinition::build_layout_gap(ctx, args[0]);
      if (!gap) return Lisple::Constant::NIL;
      return LayoutGapAdapter::claim(std::move(gap));
    }

    EXEC_BODY(MakeLayoutGap, exec_make_key)
    {
      Lisple::sptr_val_v make_args{Lisple::map({Lisple::keyword("mode"), args[0]})};
      return exec_make(ctx, make_args);
    }

    EXEC_BODY(MakeLayoutGap, exec_make_num)
    {
      Lisple::sptr_val_v make_args{Lisple::map({Lisple::keyword("mode"),
                                                Lisple::keyword("fixed"),
                                                Lisple::keyword("size"),
                                                args[0]})};
      return exec_make(ctx, make_args);
    }

    EXEC_BODY(MakeText, exec_make)
    {
      auto text = StyleDefinition::build_text(ctx, args[0]);
      if (!text) return Lisple::Constant::NIL;
      return StyleTextAdapter::claim(std::move(text));
    }

    EXEC_BODY(MakeLayout, exec_make)
    {
      auto layout = StyleDefinition::build_layout(ctx, args[0]);
      if (!layout) return Lisple::Constant::NIL;
      return LayoutAdapter::claim(std::move(layout));
    }

    EXEC_BODY(MakeStyle, exec_make)
    {
      auto style = StyleDefinition::build_style(ctx, args[0]);
      if (!style) return Lisple::Constant::NIL;
      return StyleAdapter::claim(std::move(style));
    }

    FUNC_IMPL(MakeBackground,
              MULTI_SIG((FN_ARGS((&HostType::COLOR)),
                         EXEC_DISPATCH(&MakeBackground::exec_make_color)),
                        (FN_ARGS((&Lisple::Type::KEYWORD)),
                         EXEC_DISPATCH(&MakeBackground::exec_make_image)),
                        (FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakeBackground::exec_make_map))));

    EXEC_BODY(MakeBackground, exec_make_color)
    {
      auto bg = StyleDefinition::build_background(ctx, args[0]);
      if (!bg) return Lisple::Constant::NIL;
      return BackgroundAdapter::claim(std::move(bg));
    }

    EXEC_BODY(MakeBackground, exec_make_image)
    {
      auto bg = StyleDefinition::build_background(ctx, args[0]);
      if (!bg) return Lisple::Constant::NIL;
      return BackgroundAdapter::claim(std::move(bg));
    }

    EXEC_BODY(MakeBackground, exec_make_map)
    {
      auto bg = StyleDefinition::build_background(ctx, args[0]);
      if (!bg) return Lisple::Constant::NIL;
      return BackgroundAdapter::claim(std::move(bg));
    }

    FUNC_IMPL(MakeBorder,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeBorder::exec_make))));

    EXEC_BODY(MakeBorder, exec_make)
    {
      auto border = StyleDefinition::build_border(ctx, args[0]);
      if (!border) return Lisple::Constant::NIL;
      return BorderAdapter::claim(std::move(border));
    }

    FUNC_IMPL(MakeBorderStyle,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeBorderStyle::exec_make))));

    EXEC_BODY(MakeBorderStyle, exec_make)
    {
      auto border = StyleDefinition::build_border_style(ctx, args[0]);
      if (!border) return Lisple::Constant::NIL;
      return BorderStyleAdapter::claim(std::move(border));
    }

    FUNC_IMPL(MakeInsets,
              MULTI_SIG((FN_ARGS((&Lisple::Type::NUMBER)),
                         EXEC_DISPATCH(&MakeInsets::exec_make_num)),
                        (FN_ARGS((&Lisple::Type::VECTOR_OF_NUMBER)),
                         EXEC_DISPATCH(&MakeInsets::exec_make_vec)),
                        (FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakeInsets::exec_make_map))));

    EXEC_BODY(MakeInsets, exec_make_map)
    {
      auto insets = StyleDefinition::build_insets(ctx, args[0]);
      if (!insets) return Lisple::Constant::NIL;
      return InsetsAdapter::claim(std::move(insets));
    }

    EXEC_BODY(MakeInsets, exec_make_num)
    {
      auto insets = StyleDefinition::build_insets(ctx, args[0]);
      if (!insets) return Lisple::Constant::NIL;
      return InsetsAdapter::claim(std::move(insets));
    }

    EXEC_BODY(MakeInsets, exec_make_vec)
    {
      auto insets = StyleDefinition::build_insets(ctx, args[0]);
      if (!insets) return Lisple::Constant::NIL;
      return InsetsAdapter::claim(std::move(insets));
    }
  } // namespace Function

  StyleNamespace::StyleNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__UI__STYLE))
  {
    values.emplace("make-border", Function::MakeBorder::make());
    values.emplace("make-border-style", Function::MakeBorderStyle::make());
    values.emplace("make-style", Function::MakeStyle::make());
    values.emplace("make-layout", Function::MakeLayout::make());
    values.emplace("make-layout-gap", Function::MakeLayoutGap::make());
    values.emplace("make-text", Function::MakeText::make());
    values.emplace("make-background", Function::MakeBackground::make());
    values.emplace("make-insets", Function::MakeInsets::make());
  }
} // namespace Pixils::Script
