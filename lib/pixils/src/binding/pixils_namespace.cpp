
#include "pixils/binding/pixils_namespace.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/binding/ui/style/style_adapter.h>
#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/font_registry.h>
#include <pixils/frame_events.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>

#include <SDL2/SDL_render.h>
#include <algorithm>
#include <lisple/exception.h>
#include <lisple/exec.h>
#include <lisple/form.h>
#include <lisple/host.h>
#include <lisple/host/accessor.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/exec_node.h>
#include <lisple/runtime/lower.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(ALIGN, "align");
    SHKEY(BACKGROUND, "background");
    const Lisple::sptr_rtval BLOCK = Lisple::RTValue::keyword("block");
    SHKEY(BUFFER_SIZE, "buffer-size");
    SHKEY(CHILDREN, "children");
    SHKEY(COMPOSE, "compose");
    SHKEY(DIMENSION, "dimension");
    SHKEY(DISPLAY, "display");
    SHKEY(H, "h");
    SHKEY(HELD_KEYS, "held-keys");
    SHKEY(INIT, "init");
    SHKEY(INITIAL_MODE, "initial-mode");
    SHKEY(KEY_DOWN, "key-down");
    SHKEY(MODE, "mode");
    const Lisple::sptr_rtval NAME = Lisple::RTValue::keyword("name");
    SHKEY(PIXEL_SIZE, "pixel-size");
    const Lisple::sptr_rtval PASS = Lisple::RTValue::keyword("pass");
    SHKEY(POP, "pop");
    SHKEY(PUSH, "push");
    SHKEY(QUIT, "quit");
    SHKEY(RENDER, "render");
    SHKEY(RESOLUTION, "resolution");
    SHKEY(RESOURCES, "resources");
    SHKEY(SCALING, "scaling");
    SHKEY(STATE, "state");
    SHKEY(TARGET_FRAME_RATE, "target-frame-rate");
    SHKEY(TYPE, "type");
    SHKEY(UPDATE, "update");
    SHKEY(W, "w");
  } // namespace MapKey

  namespace Key
  {
    inline const Lisple::sptr_rtval W = Lisple::RTValue::keyword("w");
    inline const Lisple::sptr_rtval H = Lisple::RTValue::keyword("h");
  } // namespace Key

  namespace Macro
  {
    /* DefBundleForm - defbundle */
    SPECIAL_FORM_IMPL(DefBundleForm,
                      SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                                   (&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&DefBundleForm::execnode_def_bundle))));

    SFORM_LOWER_IMPL(DefBundleForm)
    {
      auto name = Lisple::exec(*ctx.ctx, *lower_literal(ast_node->get_children()[1]))->str();
      auto map_expr = Lisple::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));
      auto deps_coercion = HostType::RESOURCE_DEPENDENCIES.coerce(*ctx.ctx, map_expr);
      if (!deps_coercion.success)
      {
        throw Lisple::TypeError("Invalid bundle declaration: " + map_expr->to_string());
      }

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.ctx->lookup_value(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->declare_bundle(
        name,
        Lisple::obj<Runtime::ResourceDependencies>(*deps_coercion.result));

      return std::make_unique<Lisple::ExecNode>(Lisple::Constant::NIL);
    }

    EXECNODE_BODY(DefBundleForm, execnode_def_bundle)
    {
      throw Lisple::LispleException("defbundle is lower-only");
    }

    SPECIAL_FORM_IMPL(DefFontForm,
                      SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                                   (&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&DefFontForm::execnode_def_font))));

    SFORM_LOWER_IMPL(DefFontForm)
    {
      static Lisple::MapSchema font_map_schema({},
                                               {{"type", &Lisple::Type::KEY},
                                                {"resource", &Lisple::Type::KEY},
                                                {"spacing", &Lisple::Type::NUMBER},
                                                {"line-height", &Lisple::Type::NUMBER},
                                                {"baseline", &Lisple::Type::NUMBER},
                                                {"styles", &Lisple::Type::MAP},
                                                {"glyphs", &Lisple::Type::MAP}});

      std::string font_name =
        Lisple::exec(*ctx.ctx, *lower_literal(ast_node->get_children()[1]))->str();
      if (font_name.find('/') == std::string::npos)
      {
        font_name = "font/" + font_name;
      }
      std::map<char32_t, SDL_Rect> glyph_map;
      auto font_def_map =
        Lisple::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));

      auto opts = font_map_schema.bind(*ctx.ctx, *font_def_map);
      auto type = opts.str("type", "bitmap");
      if (type != "bitmap")
      {
        throw new Lisple::InvocationException("Invalid font type: " + type);
      }
      auto resource_key = opts.val("resource");
      if (resource_key->type != Lisple::RTValue::Type::KEYWORD)
      {
        throw Lisple::TypeError("Invalid resource key: " + resource_key->to_string());
      }
      int spacing = opts.i32("spacing", 1);
      int line_height = opts.i32("line-height", 0);
      Text::FontDefinition font_definition;
      const bool has_explicit_baseline = opts.contains("baseline");
      font_definition.baseline = opts.i32("baseline", 0);

      if (opts.contains("styles"))
      {
        auto styles_value = opts.val("styles");
        if (styles_value->type != Lisple::RTValue::Type::MAP)
        {
          throw Lisple::TypeError("Font :styles must be a map");
        }

        auto underline_value =
          Lisple::Dict::get_property(styles_value, Lisple::RTValue::keyword("underline"));
        if (underline_value && underline_value->type != Lisple::RTValue::Type::NIL)
        {
          if (underline_value->type != Lisple::RTValue::Type::MAP)
          {
            throw Lisple::TypeError("Font :styles :underline must be a map");
          }

          static Lisple::MapSchema underline_schema(
            {},
            {{"offset", &Lisple::Type::NUMBER}, {"thickness", &Lisple::Type::NUMBER}});
          auto underline_opts = underline_schema.bind(*ctx.ctx, *underline_value);
          font_definition.underline =
            Text::UnderlineMetrics{.offset = underline_opts.i32("offset", 0),
                                   .thickness = underline_opts.i32("thickness", 1)};
        }
      }

      auto glyphs = opts.val("glyphs");
      if (glyphs->type == Lisple::RTValue::Type::MAP)
      {
        for (auto& ch : Lisple::Dict::keys(*glyphs))
        {
          char32_t glyph_char;
          switch (ch->type)
          {
          case Lisple::RTValue::Type::CHAR:
            glyph_char = ch->ch();
            break;
          case Lisple::RTValue::Type::STRING:
          case Lisple::RTValue::Type::KEYWORD:
          case Lisple::RTValue::Type::SYMBOL:
          {
            std::string ch_val = ch->str();
            if (ch_val.size() != 1)
            {
              throw new Lisple::TypeError("Invalid font glyph: " + ch->to_string());
            }
            glyph_char = ch_val.at(0);
            break;
          }

          default:
            throw new Lisple::TypeError("Invalid font glyph: " + ch->to_string());
          }

          auto rect_val = Lisple::Dict::get_property(glyphs, ch);
          auto glyphc = HostType::RECT.coerce(*ctx.ctx, rect_val);

          if (!glyphc.success)
          {
            throw new Lisple::TypeError("Invalid source rect for glyph " + ch->to_string() +
                                        ": " + rect_val->to_string());
          }

          glyph_map.emplace(
            glyph_char,
            glyphc.result->adapter<RectAdapter>().get_object().to_SDL_rect());
        }
      }

      if (!has_explicit_baseline)
      {
        int inferred_height = line_height;
        for (const auto& [_, rect] : glyph_map)
        {
          inferred_height = std::max(inferred_height, rect.h);
        }
        font_definition.baseline = std::max(0, inferred_height - 1);
      }

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.ctx->lookup_value(ID__PIXILS__RENDER_CONTEXT));

      auto [bundle_key, image_key] = resource_key->qual();

      SDL_Texture* resource_texture = rc.asset_registry->get_image(bundle_key, image_key);
      SDL_Texture* tint_texture = rc.asset_registry->get_tint_mask(bundle_key, image_key);

      rc.font_registry->register_font(font_name,
                                      resource_texture,
                                      tint_texture,
                                      Text::FontMap(glyph_map),
                                      font_definition,
                                      spacing,
                                      line_height);

      return std::make_unique<Lisple::ExecNode>(Lisple::Constant::NIL);
    }

    EXECNODE_BODY(DefFontForm, execnode_def_font)
    {
      throw Lisple::LispleException("Invalid invocation");
    }

    /* DefProgramForm - defprogram */
    SPECIAL_FORM_IMPL(DefProgramForm,
                      SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                                   (&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&DefProgramForm::execnode_def_program))));

    Lisple::MapSchema program_schema({},
                                     {{"display", &HostType::DISPLAY},
                                      {"initial-mode", &Lisple::Type::SYMBOL_VALUE},
                                      {"theme", &Lisple::Type::ANY},
                                      {"target-frame-rate", &Lisple::Type::NUMBER},
                                      {"pointer", &Lisple::Type::KEY}});

    SFORM_LOWER_IMPL(DefProgramForm)
    {
      auto name = Lisple::exec(*ctx.ctx, *lower_literal(ast_node->get_children()[1]))->str();
      auto map_expr = Lisple::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));

      auto opts = program_schema.bind(*ctx.ctx, *map_expr);

      auto programs = ctx.ctx->lookup_value(ID__PIXILS__PROGRAMS);
      auto initial_mode = opts.str(MapKey::INITIAL_MODE->value, "");

      Display display = opts.contains(MapKey::DISPLAY->value)
                          ? opts.obj<Display>(MapKey::DISPLAY->value)
                          : Display(Resolution(Resolution::Mode::AUTO, {0, 0}),
                                    Display::Alignment::NONE,
                                    Display::Scaling::NONE,
                                    Color(0, 0, 0));

      auto program = ProgramAdapter::make_unique(name, display, initial_mode);
      if (opts.contains("theme"))
      {
        auto theme_names = parse_theme_names(opts.val("theme"), "Program :theme");
        Lisple::obj<Program>(*program).theme =
          theme_names.empty() ? std::nullopt : std::make_optional(std::move(theme_names));
      }

      if (opts.str("pointer", "") == "off")
      {
        Lisple::obj<Program>(*program).pointer_visible = false;
      }

      if (opts.contains(MapKey::TARGET_FRAME_RATE->value))
      {
        Lisple::obj<Program>(*program).target_frame_rate =
          opts.i32(MapKey::TARGET_FRAME_RATE->value);
      }

      Lisple::Dict::set_property(programs, Lisple::RTValue::symbol(name), program);

      return std::make_unique<Lisple::ExecNode>(Lisple::Constant::NIL);
    }

    EXECNODE_BODY(DefProgramForm, execnode_def_program)
    {
      throw Lisple::LispleException("defmode is lower-only");
    }

    /* DefThemeForm - deftheme */
    SPECIAL_FORM_IMPL(DefThemeForm,
                      SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                                   (&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&DefThemeForm::execnode_declare_theme))));

    SFORM_LOWER_IMPL(DefThemeForm)
    {
      auto themes = ctx.ctx->lookup_value(ID__PIXILS__THEMES);
      auto name_expr = Lisple::exec(*ctx.ctx, *lower_literal(ast_node->get_children()[1]));
      auto name = name_expr->str();
      auto theme_expr =
        Lisple::exec(*ctx.ctx, *Lisple::lower_expr(ctx, ast_node->get_children()[2]));

      auto theme = build_theme_from_definition(*ctx.ctx, name, theme_expr);
      Lisple::Dict::set_property(themes, name_expr, ThemeAdapter::make_unique(theme));

      return std::make_unique<Lisple::ExecNode>(Lisple::Constant::NIL);
    }

    EXECNODE_BODY(DefThemeForm, execnode_declare_theme)
    {
      throw Lisple::LispleException("deftheme is lower-only");
    }

    /* DefModeForm - defmode */
    SPECIAL_FORM_IMPL(DefModeForm,
                      SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                                   (&HostType::MODE, &Lisple::Eval::LITERAL)),
                           EXEC_DISPATCH(&DefModeForm::execnode_declare_mode))));

    SFORM_LOWER_IMPL(DefModeForm)
    {
      auto modes = ctx.ctx->lookup_value(ID__PIXILS__MODES);
      auto name_expr = Lisple::exec(*ctx.ctx, *lower_literal(ast_node->get_children()[1]));
      auto name_str = Lisple::RTValue::string(name_expr->str());

      Lisple::LowerContext lctx{ctx};
      auto mode_expr =
        Lisple::exec(*ctx.ctx, *Lisple::lower_expr(lctx, ast_node->get_children()[2]));
      Lisple::Dict::set_property(mode_expr, MapKey::NAME, name_str);
      auto mode_coercion = HostType::MODE.coerce(*ctx.ctx, mode_expr);
      if (!mode_coercion.success)
      {
        throw Lisple::TypeError("Invalid mode declaration: " + mode_expr->to_string());
      }

      Lisple::Dict::set_property(modes, name_expr, mode_coercion.result);

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.ctx->lookup_value(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->declare_bundle(
        name_expr->str(),
        Lisple::obj<Runtime::Mode>(*mode_coercion.result).resources);

      return std::make_unique<Lisple::ExecNode>(Lisple::Constant::NIL);
    }

    EXECNODE_BODY(DefModeForm, execnode_declare_mode)
    {
      throw Lisple::LispleException("defmode is lower-only");
    }
  } // namespace Macro

  namespace Function
  {
    /* Mode make function */
    FUNC_IMPL(MakeMode,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeMode::exec_make))))

    EXEC_BODY(MakeMode, exec_make)
    {
      return ModeAdapter::make_unique(build_mode_from_definition(ctx, args[0]));
    }

    /* ModeComposition make function */
    FUNC_IMPL(MakeModeComposition,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeModeComposition::exec_make))));

    EXEC_BODY(MakeModeComposition, exec_make)
    {
      static Lisple::MapSchema mode_compose_schema(
        {},
        {{"render", &Lisple::Type::KEY}, {"update", &Lisple::Type::KEY}});

      auto opts = mode_compose_schema.bind(ctx, *args[0]);

      Runtime::ModeComposition composition{opts.str("render", "block") == "pass",
                                           opts.str("update", "block") == "pass"};

      return ModeCompositionAdapter::make_unique(composition);
    }

    /* Dimension make function */
    FUNC_IMPL(MakeDimension,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeDimension::exec_make))))

    Lisple::MapSchema dimension_schema({{MapKey::W->value, &Lisple::Type::NUMBER},
                                        {MapKey::H->value, &Lisple::Type::NUMBER}});

    EXEC_BODY(MakeDimension, exec_make)
    {
      auto opts = dimension_schema.bind(ctx, *args[0]);
      return DimensionAdapter::make_unique(opts.i32(MapKey::W->value),
                                           opts.i32(MapKey::H->value));
    }

    /* Display make function */
    FUNC_IMPL(MakeDisplay,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeDisplay::exec_make))))

    Lisple::MapSchema display_schema({{MapKey::RESOLUTION->value, &HostType::RESOLUTION}},
                                     {{MapKey::ALIGN->value, &Lisple::Type::KEY},
                                      {MapKey::SCALING->value, &Lisple::Type::KEY},
                                      {MapKey::BACKGROUND->value, &HostType::COLOR}});

    EXEC_BODY(MakeDisplay, exec_make)
    {
      try
      {
        auto opts = display_schema.bind(ctx, *args[0]);

        auto align = Display::Alignment::NONE;
        auto scaling = Display::Scaling::NONE;

        if (opts.contains(MapKey::ALIGN->value))
        {
          const std::string align_str = opts.str(MapKey::ALIGN->value);
          if (align_str == "align/center")
          {
            align = Display::Alignment::CENTER;
          }
        }

        if (opts.contains(MapKey::SCALING->value))
        {
          const std::string scale_str = opts.str(MapKey::SCALING->value);
          if (scale_str == "scaling/stretch")
          {
            scaling = Display::Scaling::STRETCH;
          }
          else if (scale_str == "scaling/fit")
          {
            scaling = Display::Scaling::FIT;
          }
        }

        Color color = opts.obj<Color>(MapKey::BACKGROUND->value, Color{0, 0, 0});

        return DisplayAdapter::make_unique(opts.obj<Resolution>(MapKey::RESOLUTION->value),
                                           align,
                                           scaling,
                                           color);
      }
      catch (std::exception& e)
      {
        throw e;
      }
    }

    /* Resolution make function */
    FUNC_IMPL(MakeResolution,
              MULTI_SIG((FN_ARGS((&HostType::DIMENSION)),
                         EXEC_DISPATCH(&MakeResolution::exec_make_resolution)),
                        (FN_ARGS((&Lisple::Type::KEY)),
                         EXEC_DISPATCH(&MakeResolution::exec_make_resolution)),
                        (FN_ARGS((&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&MakeResolution::exec_make_resolution))));

    EXEC_BODY(MakeResolution, exec_make_resolution)
    {
      if (Lisple::Type::KEY.is_type_of(*args[0]))
      {
        const std::string& res_type = args[0]->str();
        if (res_type == "auto")
        {
          return ResolutionAdapter::make_unique(Resolution::Mode::AUTO);
        }
        throw Lisple::TypeError("Invalid resolution specifier: " + args[0]->to_string());
      }
      else if (Lisple::Type::MAP.is_type_of(*args[0]))
      {
        auto scale_val =
          Lisple::Dict::get_property(args[0], Lisple::RTValue::keyword("scale"));
        if (scale_val && scale_val->type == Lisple::RTValue::Type::NUMBER)
        {
          int ps = scale_val->num().get_int();
          return ResolutionAdapter::make_unique(Resolution::Mode::AUTO, ps);
        }

        Lisple::CoercionResult<Lisple::RTValue> cresult =
          HostType::DIMENSION.coerce(ctx, args[0]);
        if (cresult.success)
        {
          return ResolutionAdapter::make_unique(Resolution::Mode::FIXED,
                                                Lisple::obj<Dimension>(*cresult.result));
        }
      }

      throw Lisple::TypeError("Could not construct Resolution from: " +
                              args[0]->to_string());
    }

    /* PushModeBangFunction - push-mode! */
    FUNC_IMPL(PushModeBangFunction,
              MULTI_SIG((FN_ARGS((&Lisple::Type::SYMBOL_VALUE)),
                         EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode)),
                        (FN_ARGS((&Lisple::Type::SYMBOL_VALUE), (&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode)),
                        (FN_ARGS((&Lisple::Type::SYMBOL_VALUE),
                                 (&Lisple::Type::ANY),
                                 (&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode))));

    EXEC_BODY(PushModeBangFunction, exec_push_mode)
    {
      auto message_queue = ctx.lookup_value(ID__PIXILS__MODE_STACK_MESSAGES);

      Lisple::append(
        *message_queue,
        Lisple::RTValue::map({Lisple::RTValue::keyword(MapKey::TYPE->value),
                              Lisple::RTValue::keyword(MapKey::PUSH->value),
                              Lisple::RTValue::keyword(MapKey::MODE->value),
                              args.front(),
                              Lisple::RTValue::keyword(MapKey::STATE->value),
                              args.size() > 1 ? args[1] : Lisple::Constant::NIL,
                              Lisple::RTValue::keyword("overrides"),
                              args.size() > 2 ? args[2] : Lisple::Constant::NIL}));

      return args[0];
    }

    /* PopModeBangFunction - pop-mode! */
    FUNC_IMPL(PopModeBangFunction,
              MULTI_SIG((NO_ARGS, EXEC_DISPATCH(&PopModeBangFunction::exec_pop_mode)),
                        (FN_ARGS((&Lisple::Type::ANY)),
                         EXEC_DISPATCH(&PopModeBangFunction::exec_pop_mode))));

    EXEC_BODY(PopModeBangFunction, exec_pop_mode)
    {
      auto message_queue = ctx.lookup_value(ID__PIXILS__MODE_STACK_MESSAGES);

      Lisple::append(*message_queue,
                     Lisple::RTValue::map({
                       Lisple::RTValue::keyword(MapKey::TYPE->value),
                       Lisple::RTValue::keyword(MapKey::POP->value),
                       Lisple::RTValue::keyword("payload"),
                       args.empty() ? Lisple::Constant::NIL : args[0],
                     }));

      return Lisple::Constant::NIL;
    }

    /* QuitBangFunction - quit! */
    FUNC_IMPL(QuitBangFunction, SIG((NO_ARGS, EXEC_DISPATCH(&QuitBangFunction::exec_quit))));

    EXEC_BODY(QuitBangFunction, exec_quit)
    {
      auto message_queue = ctx.lookup_value(ID__PIXILS__MODE_STACK_MESSAGES);

      Lisple::append(*message_queue,
                     Lisple::RTValue::map({
                       Lisple::RTValue::keyword(MapKey::TYPE->value),
                       Lisple::RTValue::keyword(MapKey::QUIT->value),
                     }));

      return Lisple::Constant::NIL;
    }

  } // namespace Function

  /* ModeAdapter */
  NATIVE_ADAPTER_IMPL(ModeAdapter,
                      Runtime::Mode,
                      &HostType::MODE,
                      (init),
                      (update),
                      (render));

  NOBJ_PROP_GET(ModeAdapter, init)
  {
    return this->get_object().init;
  }

  NOBJ_PROP_GET(ModeAdapter, update)
  {
    return this->get_object().update;
  }

  NOBJ_PROP_GET(ModeAdapter, render)
  {
    return this->get_object().render;
  }

  /* ModeCompositionAdapter */
  NATIVE_ADAPTER_IMPL(ModeCompositionAdapter,
                      Runtime::ModeComposition,
                      &HostType::MODE_COMPOSITION,
                      (render));

  Lisple::sptr_rtval ModeCompositionAdapter::get_render() const
  {
    return this->get_object().render ? MapKey::PASS : MapKey::BLOCK;
  }

  /* DimensionAdapter */
  NATIVE_ADAPTER_IMPL(DimensionAdapter, Dimension, &HostType::DIMENSION, (w), (h));

  NOBJ_PROP_GET_SET__FIELD(DimensionAdapter, w);
  NOBJ_PROP_GET_SET__FIELD(DimensionAdapter, h);

  /* RectAdapter */
  NATIVE_ADAPTER_IMPL(RectAdapter,
                      Rect,
                      &HostType::RECT,
                      (rw, "x", x),
                      (rw, "y", y),
                      (rw, "w", w),
                      (rw, "h", h))

  NOBJ_PROP_GET_SET__FIELD(RectAdapter, x)
  NOBJ_PROP_GET_SET__FIELD(RectAdapter, y)
  NOBJ_PROP_GET_SET__FIELD(RectAdapter, w)
  NOBJ_PROP_GET_SET__FIELD(RectAdapter, h)

  /* FrameEventsAdapter */
  NATIVE_ADAPTER_IMPL(FrameEventsAdapter,
                      FrameEvents,
                      &HostType::FRAME_EVENTS,
                      ("key-down", key_down),
                      ("held-keys", held_keys));

  NOBJ_PROP_GET(FrameEventsAdapter, key_down)
  {
    return object->get_object().key_down;
  }

  NOBJ_PROP_GET(FrameEventsAdapter, held_keys)
  {
    return object->get_object().held_keys;
  }

  /* HookContextAdapter */
  NATIVE_ADAPTER_IMPL(HookContextAdapter,
                      HookContext,
                      &HostType::HOOK_CONTEXT,
                      ("key-down", key_down),
                      ("held-keys", held_keys),
                      ("mouse-pos", mouse_pos),
                      ("mouse-button-down", mouse_button_down),
                      ("mouse-button-up", mouse_button_up),
                      ("mouse-held", mouse_held),
                      ("pixel-size", pixel_size),
                      ("buffer-size", buffer_dim),
                      ("available-width", available_width),
                      ("available-height", available_height),
                      (view));

  NOBJ_PROP_GET(HookContextAdapter, key_down)
  {
    return object->get_object().events->key_down;
  }

  NOBJ_PROP_GET(HookContextAdapter, held_keys)
  {
    return object->get_object().events->held_keys;
  }

  NOBJ_PROP_GET(HookContextAdapter, mouse_pos)
  {
    return object->get_object().events->mouse_pos;
  }

  NOBJ_PROP_GET(HookContextAdapter, mouse_button_down)
  {
    return object->get_object().events->mouse_button_down;
  }

  NOBJ_PROP_GET(HookContextAdapter, mouse_button_up)
  {
    return object->get_object().events->mouse_button_up;
  }

  NOBJ_PROP_GET(HookContextAdapter, mouse_held)
  {
    return object->get_object().events->mouse_held;
  }

  NOBJ_PROP_GET(HookContextAdapter, pixel_size)
  {
    return Lisple::RTValue::number(object->get_object().render->pixel_size);
  }

  NOBJ_PROP_GET(HookContextAdapter, buffer_dim)
  {
    const Dimension& dim = object->get_object().render->buffer_dim;
    return DimensionAdapter::make_unique(dim.w, dim.h);
  }

  NOBJ_PROP_GET(HookContextAdapter, available_width)
  {
    auto available = object->get_object().available_width;
    return available ? Lisple::RTValue::number(*available) : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(HookContextAdapter, available_height)
  {
    auto available = object->get_object().available_height;
    return available ? Lisple::RTValue::number(*available) : Lisple::Constant::NIL;
  }

  NOBJ_PROP_GET(HookContextAdapter, view)
  {
    auto view_ptr = object->get_object().current_view;
    if (!view_ptr) return Lisple::Constant::NIL;
    return ViewAdapter::make_ref(*view_ptr);
  }

  /* InteractionStateAdapter */
  NATIVE_ADAPTER_IMPL(InteractionStateAdapter,
                      UI::InteractionState,
                      &HostType::INTERACTION_STATE,
                      (hovered),
                      (focused),
                      ("focus-within", focus_within),
                      (pressed));

  NOBJ_PROP_GET__FIELD(InteractionStateAdapter, hovered);
  NOBJ_PROP_GET__FIELD(InteractionStateAdapter, focused);
  NOBJ_PROP_GET__FIELD(InteractionStateAdapter, focus_within);

  NOBJ_PROP_GET(InteractionStateAdapter, pressed)
  {
    Lisple::sptr_rtval_v buttons;
    for (UI::MouseButton btn : object->get_object().pressed)
      buttons.push_back(Lisple::RTValue::keyword(UI::mouse_button_name(btn)));
    return Lisple::RTValue::vector(buttons);
  }

  /* ViewAdapter */
  NATIVE_ADAPTER_IMPL(ViewAdapter,
                      Runtime::View,
                      &HostType::VIEW,
                      (id),
                      (bounds),
                      (interaction),
                      (style),
                      ("effective-style", effective_style),
                      ("on-click", on_click),
                      ("on-mouse-down", on_mouse_down),
                      ("on-mouse-up", on_mouse_up));

  NOBJ_PROP_GET__FIELD(ViewAdapter, id);
  NOBJ_PROP_GET(ViewAdapter, bounds)
  {
    const Rect& b = object->get_object().bounds;
    return RectAdapter::make_unique(b.x, b.y, b.w, b.h);
  }

  NOBJ_PROP_GET(ViewAdapter, interaction)
  {
    return InteractionStateAdapter::make_unique(object->get_object().interaction);
  }

  NOBJ_PROP_GET(ViewAdapter, style)
  {
    const Runtime::View& v = object->get_object();
    if (!v.mode || !v.mode->style.has_value()) return Lisple::Constant::NIL;
    return StyleAdapter::make_ref(*v.mode->style);
  }

  NOBJ_PROP_GET(ViewAdapter, effective_style)
  {
    return StyleAdapter::make_ref(object->get_object().effective_style);
  }

  NOBJ_PROP_GET(ViewAdapter, on_click)
  {
    const Runtime::View& v = object->get_object();
    return v.owned_mode && *v.owned_mode->on_click != *Lisple::Constant::NIL
             ? v.owned_mode->on_click
             : v.mode->on_click;
  }

  NOBJ_PROP_GET(ViewAdapter, on_mouse_up)
  {
    const Runtime::View& v = object->get_object();
    return v.owned_mode && *v.owned_mode->on_mouse_up != *Lisple::Constant::NIL
             ? v.owned_mode->on_mouse_up
             : v.mode->on_mouse_up;
  }

  NOBJ_PROP_GET(ViewAdapter, on_mouse_down)
  {
    const Runtime::View& v = object->get_object();
    return v.owned_mode && *v.owned_mode->on_mouse_down != *Lisple::Constant::NIL
             ? v.owned_mode->on_mouse_down
             : v.mode->on_mouse_down;
  }

  /* RenderContextAdapter */
  NATIVE_ADAPTER_IMPL(RenderContextAdapter,
                      RenderContext,
                      &HostType::RENDER_CONTEXT,
                      ("pixel-size", pixel_size),
                      ("buffer-size", buffer_dim));

  NOBJ_PROP_GET__FIELD(RenderContextAdapter, pixel_size);
  NOBJ_PROP_GET_ADAPTER__FIELD(RenderContextAdapter, buffer_dim, DimensionAdapter);

  /* ProgramAdapter */
  NATIVE_ADAPTER_IMPL(ProgramAdapter,
                      Program,
                      &HostType::PROGRAM,
                      (name),
                      ("initial-mode", initial_mode),
                      (display),
                      (theme),
                      ("target-frame-rate", target_frame_rate));

  NOBJ_PROP_GET__METHOD(ProgramAdapter, name);
  NOBJ_PROP_GET_SET_ADAPTER__FIELD(ProgramAdapter, display, DisplayAdapter);

  NOBJ_PROP_GET(ProgramAdapter, initial_mode)
  {
    if (get_object().initial_mode == "")
    {
      return Lisple::Constant::NIL;
    }
    return Lisple::RTValue::symbol(get_object().initial_mode);
  };

  NOBJ_PROP_GET(ProgramAdapter, theme)
  {
    if (!get_object().theme) return Lisple::Constant::NIL;
    const auto& theme_names = *get_object().theme;
    if (theme_names.size() == 1)
    {
      return Lisple::RTValue::symbol(theme_names[0]);
    }

    std::vector<Lisple::sptr_rtval> values;
    values.reserve(theme_names.size());
    for (const auto& theme_name : theme_names)
    {
      values.push_back(Lisple::RTValue::symbol(theme_name));
    }
    return Lisple::RTValue::vector(values);
  }

  NOBJ_PROP_GET__FIELD(ProgramAdapter, target_frame_rate);

  /* DisplayAdapter */
  NATIVE_ADAPTER_IMPL(DisplayAdapter, Display, &HostType::DISPLAY, (resolution));

  NOBJ_PROP_GET_SET_ADAPTER__FIELD(DisplayAdapter, resolution, ResolutionAdapter);

  /* ResolutionAdapter */
  NATIVE_ADAPTER_IMPL(ResolutionAdapter, Resolution, &HostType::RESOLUTION, (dimension));

  NOBJ_PROP_GET_ADAPTER__FIELD(ResolutionAdapter, dimension, DimensionAdapter);

  PixilsNamespace::PixilsNamespace(const RenderContext& render_context)
    : Lisple::Namespace(NS_PIXILS)
  {
    values.emplace("mode-stack", Lisple::RTValue::vector({}));
    values.emplace("mode-stack-messages", Lisple::RTValue::vector({}));
    values.emplace("modes", Lisple::RTValue::map({}));
    values.emplace("themes", Lisple::RTValue::map({}));
    values.emplace("defbundle", Macro::DefBundleForm::make());
    values.emplace("defmode", Macro::DefModeForm::make());
    values.emplace("defcomponent", Macro::DefModeForm::make());
    values.emplace("deftheme", Macro::DefThemeForm::make());
    values.emplace("deffont", Macro::DefFontForm::make());
    values.emplace("defprogram", Macro::DefProgramForm::make());
    values.emplace("make-dimension", Function::MakeDimension::make());
    values.emplace("make-display", Function::MakeDisplay::make());
    values.emplace("make-mode", Function::MakeMode::make());
    values.emplace("make-mode-composition", Function::MakeModeComposition::make());
    values.emplace("make-resolution", Function::MakeResolution::make());
    values.emplace("render-context", RenderContextAdapter::make_ref(render_context));
    values.emplace("programs", Lisple::RTValue::map({}));
    values.emplace("pop-mode!", Function::PopModeBangFunction::make());
    values.emplace(FN__PUSH_MODE_BANG, Function::PushModeBangFunction::make());
    values.emplace(FN__QUIT_BANG, Function::QuitBangFunction::make());
  }

} // namespace Pixils::Script
