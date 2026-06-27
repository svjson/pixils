
#include "pixils/binding/pixils_namespace.h"

#include <pixils/asset/embedded_assets.h>
#include <pixils/asset/registry.h>
#include <pixils/binding/color_namespace.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/binding/ui/style/style_adapter.h>
#include <pixils/binding/ui/style/style_definition.h>
#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/font_registry.h>
#include <pixils/frame_events.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_geometry.h>

#include <SDL2/SDL_render.h>
#include <algorithm>
#include <roo/exception.h>
#include <roo/exec.h>
#include <roo/form.h>
#include <roo/host/accessor.h>
#include <roo/host/schema.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/exec_node.h>
#include <roo/runtime/lower.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(ALIGN, "align");
    SHKEY(BACKGROUND, "background");
    const Roo::sptr_val BLOCK = Roo::keyword("block");
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
    const Roo::sptr_val NAME = Roo::keyword("name");
    SHKEY(PIXEL_SIZE, "pixel-size");
    const Roo::sptr_val PASS = Roo::keyword("pass");
    SHKEY(POP, "pop");
    SHKEY(PUSH, "push");
    SHKEY(QUIT, "quit");
    SHKEY(RENDER, "render");
    SHKEY(RESOLUTION, "resolution");
    SHKEY(RESOURCES, "resources");
    SHKEY(SCALING, "scaling");
    SHKEY(STATE, "state");
    SHKEY(TARGET_FRAME_RATE, "target-frame-rate");
    SHKEY(THEME, "theme");
    SHKEY(THEME_VARIANT, "theme-variant");
    SHKEY(TYPE, "type");
    SHKEY(UPDATE, "update");
    SHKEY(W, "w");
  } // namespace MapKey

  namespace Key
  {
    inline const Roo::sptr_val W = Roo::keyword("w");
    inline const Roo::sptr_val H = Roo::keyword("h");
  } // namespace Key

  namespace
  {
    void set_runtime_map_key_property(Roo::sptr_val& target,
                                      const Roo::sptr_val& property,
                                      const Roo::sptr_val& value)
    {
      if (target->type != Roo::Value::Type::MAP)
      {
        Roo::Dict::set_property(target, property, value);
        return;
      }

      auto& elements = target->mut_elements();
      if (elements.size() % 2 != 0)
      {
        throw Roo::RooException("Invalid map structure");
      }

      for (size_t i = 0; i < elements.size(); i += 2)
      {
        if (*elements[i] == *property)
        {
          elements[i + 1] = value;
          return;
        }
      }

      elements.push_back(property);
      elements.push_back(value);
    }

    void fill_missing_external_geometry_style(UI::Style& style, const UI::Style& source)
    {
      if (!style.width && source.width) style.width = source.width;
      if (!style.height && source.height) style.height = source.height;
      if (!style.scale && source.scale) style.scale = source.scale;
    }

    UI::Style best_known_external_geometry_style(const Runtime::View& view)
    {
      UI::Style style = view.effective_style;
      if (view.mode)
      {
        if (view.mode->style) fill_missing_external_geometry_style(style, *view.mode->style);
        if (view.mode->runtime_style)
          fill_missing_external_geometry_style(style, *view.mode->runtime_style);
      }
      return style;
    }

    Rect best_known_external_bounds(const Runtime::View& view)
    {
      if (view.external_bounds.w > 0 && view.external_bounds.h > 0)
        return view.external_bounds;

      UI::Style style = best_known_external_geometry_style(view);
      Rect logical_bounds = view.bounds;
      if (logical_bounds.w <= 0 && style.width)
        logical_bounds.w = style.width->fixed_value_or(0);
      if (logical_bounds.h <= 0 && style.height)
        logical_bounds.h = style.height->fixed_value_or(0);
      return UI::scaled_external_bounds(logical_bounds, style);
    }
  } // namespace

  namespace Macro
  {
    /* DefPointerForm - defpointer */
    SPECIAL_FORM_IMPL(DefPointerForm,
                      SIG((FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL),
                                   (&Roo::Type::MAP)),
                           EXEC_DISPATCH(&DefPointerForm::execnode_def_pointer))));

    SFORM_LOWER_IMPL(DefPointerForm)
    {
      auto name =
        Roo::exec(*ctx.ctx, *Roo::lower_literal(ast_node->get_children()[1]))->str();
      auto map_expr = Roo::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));
      auto pointer = StyleDefinition::parse_image_cursor(*ctx.ctx, map_expr);
      if (!pointer)
      {
        throw Roo::TypeError("Invalid pointer declaration: " + map_expr->to_string());
      }

      RenderContext& rc =
        Roo::obj<RenderContext>(*ctx.ctx->lookup(ID__PIXILS__RENDER_CONTEXT));
      rc.pointer_registry[name] = *pointer;

      return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
    }

    EXECNODE_BODY(DefPointerForm, execnode_def_pointer)
    {
      throw Roo::RooException("defpointer is lower-only");
    }

    /* DefBundleForm - defbundle */
    SPECIAL_FORM_IMPL(DefBundleForm,
                      SIG((FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL),
                                   (&Roo::Type::MAP)),
                           EXEC_DISPATCH(&DefBundleForm::execnode_def_bundle))));

    SFORM_LOWER_IMPL(DefBundleForm)
    {
      auto name =
        Roo::exec(*ctx.ctx, *Roo::lower_literal(ast_node->get_children()[1]))->str();
      auto map_expr = Roo::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));
      auto deps_coercion = HostType::RESOURCE_DEPENDENCIES.coerce(*ctx.ctx, map_expr);
      if (!deps_coercion.success)
      {
        throw Roo::TypeError("Invalid bundle declaration: " + map_expr->to_string());
      }

      RenderContext& rc =
        Roo::obj<RenderContext>(*ctx.ctx->lookup(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->declare_bundle(
        name,
        Roo::obj<Runtime::ResourceDependencies>(*deps_coercion.result));

      return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
    }

    EXECNODE_BODY(DefBundleForm, execnode_def_bundle)
    {
      throw Roo::RooException("defbundle is lower-only");
    }

    /* DefBundleDynamicForm - defbundle-dynamic */
    SPECIAL_FORM_IMPL(
      DefBundleDynamicForm,
      MULTI_SIG((FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL)),
                 EXEC_DISPATCH(&DefBundleDynamicForm::execnode_def_bundle_dynamic)),
                (FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL), (&Roo::Type::MAP)),
                 EXEC_DISPATCH(&DefBundleDynamicForm::execnode_def_bundle_dynamic))));

    SFORM_LOWER_IMPL(DefBundleDynamicForm)
    {
      auto name =
        Roo::exec(*ctx.ctx, *Roo::lower_literal(ast_node->get_children()[1]))->str();

      Runtime::ResourceDependencies deps;
      if (ast_node->get_children().size() > 2)
      {
        auto map_expr = Roo::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));
        auto deps_coercion = HostType::RESOURCE_DEPENDENCIES.coerce(*ctx.ctx, map_expr);
        if (!deps_coercion.success)
        {
          throw Roo::TypeError("Invalid dynamic bundle declaration: " +
                               map_expr->to_string());
        }
        deps = Roo::obj<Runtime::ResourceDependencies>(*deps_coercion.result);
      }

      RenderContext& rc =
        Roo::obj<RenderContext>(*ctx.ctx->lookup(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->declare_bundle(name, deps, true);

      return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
    }

    EXECNODE_BODY(DefBundleDynamicForm, execnode_def_bundle_dynamic)
    {
      throw Roo::RooException("defbundle-dynamic is lower-only");
    }

    SPECIAL_FORM_IMPL(DefFontForm,
                      SIG((FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL),
                                   (&Roo::Type::MAP)),
                           EXEC_DISPATCH(&DefFontForm::execnode_def_font))));

    SFORM_LOWER_IMPL(DefFontForm)
    {
      static Roo::MapSchema font_map_schema({},
                                            {{"type", &Roo::Type::KEYWORD},
                                             {"resource", &Roo::Type::KEYWORD},
                                             {"size", &Roo::Type::NUMBER},
                                             {"spacing", &Roo::Type::NUMBER},
                                             {"line-height", &Roo::Type::NUMBER},
                                             {"baseline", &Roo::Type::NUMBER},
                                             {"styles", &Roo::Type::MAP},
                                             {"glyphs", &Roo::Type::MAP}});

      std::string font_name =
        Roo::exec(*ctx.ctx, *Roo::lower_literal(ast_node->get_children()[1]))->str();
      if (font_name.find('/') == std::string::npos)
      {
        font_name = "font/" + font_name;
      }
      std::map<char32_t, SDL_Rect> glyph_map;
      auto font_def_map = Roo::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));

      auto opts = font_map_schema.bind(*ctx.ctx, *font_def_map);
      auto type = opts.str("type", "bitmap");
      if (type != "bitmap" && type != "ttf")
      {
        throw Roo::InvocationException("Invalid font type: " + type);
      }
      auto resource_key = opts.val("resource");
      if (resource_key->type != Roo::Value::Type::KEYWORD)
      {
        throw Roo::TypeError("Invalid resource key: " + resource_key->to_string());
      }
      int spacing = opts.i32("spacing", 1);
      int line_height = opts.i32("line-height", 0);
      Text::FontDefinition font_definition;
      const bool has_explicit_baseline = opts.contains("baseline");
      font_definition.baseline = opts.i32("baseline", 0);

      if (opts.contains("styles"))
      {
        auto styles_value = opts.val("styles");
        if (styles_value->type != Roo::Value::Type::MAP)
        {
          throw Roo::TypeError("Font :styles must be a map");
        }

        auto underline_value =
          Roo::Dict::get_property(styles_value, Roo::keyword("underline"));
        if (underline_value && underline_value->type != Roo::Value::Type::NIL)
        {
          if (underline_value->type != Roo::Value::Type::MAP)
          {
            throw Roo::TypeError("Font :styles :underline must be a map");
          }

          static Roo::MapSchema underline_schema(
            {},
            {{"offset", &Roo::Type::NUMBER}, {"thickness", &Roo::Type::NUMBER}});
          auto underline_opts = underline_schema.bind(*ctx.ctx, *underline_value);
          font_definition.underline =
            Text::UnderlineMetrics{.offset = underline_opts.i32("offset", 0),
                                   .thickness = underline_opts.i32("thickness", 1)};
        }
      }

      RenderContext& rc =
        Roo::obj<RenderContext>(*ctx.ctx->lookup(ID__PIXILS__RENDER_CONTEXT));

      auto [bundle_key, resource_id] = resource_key->qual();

      if (type == "ttf")
      {
        int size = opts.i32("size", line_height > 0 ? line_height : 16);
        auto font_path = rc.asset_registry->get_font_path(bundle_key, resource_id);
        const Assets::EmbeddedAsset* embedded_font =
          font_path ? nullptr
                    : rc.asset_registry->get_embedded_font(bundle_key, resource_id);
        if (!font_path && !embedded_font)
        {
          throw Roo::InvocationException("Unknown font resource: " +
                                         resource_key->to_string());
        }
        if (!rc.renderer)
        {
          return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
        }

        bool registered =
          font_path ? rc.font_registry->register_ttf_font(font_name,
                                                          rc.renderer,
                                                          *font_path,
                                                          size,
                                                          font_definition,
                                                          spacing,
                                                          line_height,
                                                          !has_explicit_baseline)
                    : rc.font_registry->register_ttf_font_data(font_name,
                                                               rc.renderer,
                                                               embedded_font->data,
                                                               embedded_font->size,
                                                               size,
                                                               font_definition,
                                                               spacing,
                                                               line_height,
                                                               !has_explicit_baseline);
        if (!registered)
        {
          throw Roo::InvocationException("Could not load TTF font: " +
                                         resource_key->to_string());
        }

        return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
      }

      auto glyphs = opts.val("glyphs");
      if (glyphs->type == Roo::Value::Type::MAP)
      {
        for (auto& ch : Roo::Dict::keys(*glyphs))
        {
          char32_t glyph_char;
          switch (ch->type)
          {
          case Roo::Value::Type::CHAR:
            glyph_char = ch->ch();
            break;
          case Roo::Value::Type::STRING:
          case Roo::Value::Type::KEYWORD:
          case Roo::Value::Type::SYMBOL:
          {
            std::string ch_val = ch->str();
            if (ch_val.size() != 1)
            {
              throw new Roo::TypeError("Invalid font glyph: " + ch->to_string());
            }
            glyph_char = ch_val.at(0);
            break;
          }

          default:
            throw new Roo::TypeError("Invalid font glyph: " + ch->to_string());
          }

          auto rect_val = Roo::Dict::get_property(glyphs, ch);
          auto glyphc = HostType::RECT.coerce(*ctx.ctx, rect_val);

          if (!glyphc.success)
          {
            throw new Roo::TypeError("Invalid source rect for glyph " + ch->to_string() +
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

      SDL_Texture* resource_texture = rc.asset_registry->get_image(bundle_key, resource_id);
      SDL_Texture* tint_texture = rc.asset_registry->get_tint_mask(bundle_key, resource_id);

      rc.font_registry->register_font(font_name,
                                      resource_texture,
                                      tint_texture,
                                      Text::FontMap(glyph_map),
                                      font_definition,
                                      spacing,
                                      line_height);

      return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
    }

    EXECNODE_BODY(DefFontForm, execnode_def_font)
    {
      throw Roo::RooException("Invalid invocation");
    }

    /* DefProgramForm - defprogram */
    SPECIAL_FORM_IMPL(DefProgramForm,
                      SIG((FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL),
                                   (&Roo::Type::MAP)),
                           EXEC_DISPATCH(&DefProgramForm::execnode_def_program))));

    Roo::MapSchema program_schema({},
                                  {{"display", &HostType::DISPLAY},
                                   {"initial-mode", &Roo::Type::SYMBOL_VALUE},
                                   {"theme", &Roo::Type::ANY},
                                   {"theme-variant", &Roo::Type::ANY},
                                   {"target-frame-rate", &Roo::Type::NUMBER},
                                   {"pointer", &Roo::Type::KEYWORD}});

    SFORM_LOWER_IMPL(DefProgramForm)
    {
      auto name =
        Roo::exec(*ctx.ctx, *Roo::lower_literal(ast_node->get_children()[1]))->str();
      auto map_expr = Roo::exec(*ctx.ctx, *lower_expr(ctx, ast_node->get_children()[2]));

      auto opts = program_schema.bind(*ctx.ctx, *map_expr);

      auto programs = ctx.ctx->lookup(ID__PIXILS__PROGRAMS);
      auto initial_mode = opts.str(std::get<std::string>(MapKey::INITIAL_MODE->value), "");

      Display display = opts.contains(std::get<std::string>(MapKey::DISPLAY->value))
                          ? opts.obj<Display>(std::get<std::string>(MapKey::DISPLAY->value))
                          : Display(Resolution(Resolution::Mode::AUTO, {0, 0}),
                                    Display::Alignment::NONE,
                                    Display::Scaling::NONE,
                                    Color(0, 0, 0));

      auto program = ProgramAdapter::make_unique(name, display, initial_mode);
      if (opts.contains("theme"))
      {
        auto theme_names = parse_theme_names(opts.val("theme"), "Program :theme");
        Roo::obj<Program>(*program).theme =
          theme_names.empty() ? std::nullopt : std::make_optional(std::move(theme_names));
      }

      if (opts.contains("theme-variant"))
      {
        Roo::obj<Program>(*program).theme_variant =
          parse_theme_variant(opts.val("theme-variant"), "Program :theme-variant");
      }

      if (opts.str("pointer", "") == "off")
      {
        Roo::obj<Program>(*program).pointer_visible = false;
      }

      if (opts.contains(std::get<std::string>(MapKey::TARGET_FRAME_RATE->value)))
      {
        Roo::obj<Program>(*program).target_frame_rate =
          opts.i32(std::get<std::string>(MapKey::TARGET_FRAME_RATE->value));
      }

      set_runtime_map_key_property(programs, Roo::symbol(name), program);

      return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
    }

    EXECNODE_BODY(DefProgramForm, execnode_def_program)
    {
      throw Roo::RooException("defmode is lower-only");
    }

    /* DefThemeForm - deftheme */
    SPECIAL_FORM_IMPL(DefThemeForm,
                      SIG((FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL),
                                   (&Roo::Type::MAP)),
                           EXEC_DISPATCH(&DefThemeForm::execnode_declare_theme))));

    SFORM_LOWER_IMPL(DefThemeForm)
    {
      auto themes = ctx.ctx->lookup(ID__PIXILS__THEMES);
      auto name_expr = Roo::exec(*ctx.ctx, *Roo::lower_literal(ast_node->get_children()[1]));
      auto name = name_expr->str();
      auto theme_expr =
        Roo::exec(*ctx.ctx, *Roo::lower_expr(ctx, ast_node->get_children()[2]));

      auto theme = build_theme_from_definition(*ctx.ctx, name, theme_expr);
      set_runtime_map_key_property(themes, name_expr, ThemeAdapter::make_unique(theme));

      return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
    }

    EXECNODE_BODY(DefThemeForm, execnode_declare_theme)
    {
      throw Roo::RooException("deftheme is lower-only");
    }

    /* DefModeForm - defmode */
    SPECIAL_FORM_IMPL(DefModeForm,
                      SIG((FN_ARGS((&Roo::Type::SYMBOL, &Roo::Eval::LITERAL),
                                   (&HostType::MODE, &Roo::Eval::LITERAL)),
                           EXEC_DISPATCH(&DefModeForm::execnode_declare_mode))));

    SFORM_LOWER_IMPL(DefModeForm)
    {
      auto modes = ctx.ctx->lookup(ID__PIXILS__MODES);
      auto name_expr = Roo::exec(*ctx.ctx, *Roo::lower_literal(ast_node->get_children()[1]));
      auto name_str = Roo::string(name_expr->str());

      Roo::LowerContext lctx{ctx};
      auto mode_expr =
        Roo::exec(*ctx.ctx, *Roo::lower_expr(lctx, ast_node->get_children()[2]));
      set_runtime_map_key_property(mode_expr, MapKey::NAME, name_str);
      auto mode_coercion = HostType::MODE.coerce(*ctx.ctx, mode_expr);
      if (!mode_coercion.success)
      {
        throw Roo::TypeError("Invalid mode declaration: " + mode_expr->to_string());
      }

      set_runtime_map_key_property(modes, name_expr, mode_coercion.result);

      RenderContext& rc =
        Roo::obj<RenderContext>(*ctx.ctx->lookup(ID__PIXILS__RENDER_CONTEXT));
      rc.asset_registry->declare_bundle(
        name_expr->str(),
        Roo::obj<Runtime::Mode>(*mode_coercion.result).resources);

      return std::make_unique<Roo::ExecNode>(Roo::Constant::NIL);
    }

    EXECNODE_BODY(DefModeForm, execnode_declare_mode)
    {
      throw Roo::RooException("defmode is lower-only");
    }
  } // namespace Macro

  namespace Function
  {
    /* Mode make function */
    FUNC_IMPL(MakeMode,
              SIG((FN_ARGS((&Roo::Type::MAP)), EXEC_DISPATCH(&MakeMode::exec_make))))

    EXEC_BODY(MakeMode, exec_make)
    {
      return ModeAdapter::make_unique(build_mode_from_definition(ctx, args[0]));
    }

    /* ModeComposition make function */
    FUNC_IMPL(MakeModeComposition,
              SIG((FN_ARGS((&Roo::Type::MAP)),
                   EXEC_DISPATCH(&MakeModeComposition::exec_make))));

    EXEC_BODY(MakeModeComposition, exec_make)
    {
      static Roo::MapSchema mode_compose_schema(
        {},
        {{"render", &Roo::Type::KEYWORD}, {"update", &Roo::Type::KEYWORD}});

      auto opts = mode_compose_schema.bind(ctx, *args[0]);

      Runtime::ModeComposition composition{opts.str("render", "block") == "pass",
                                           opts.str("update", "block") == "pass"};

      return ModeCompositionAdapter::make_unique(composition);
    }

    /* Dimension make function */
    FUNC_IMPL(MakeDimension,
              SIG((FN_ARGS((&Roo::Type::MAP)), EXEC_DISPATCH(&MakeDimension::exec_make))))

    Roo::MapSchema dimension_schema(
      {{std::get<std::string>(MapKey::W->value), &Roo::Type::NUMBER},
       {std::get<std::string>(MapKey::H->value), &Roo::Type::NUMBER}});

    EXEC_BODY(MakeDimension, exec_make)
    {
      auto opts = dimension_schema.bind(ctx, *args[0]);
      return DimensionAdapter::make_unique(
        opts.i32(std::get<std::string>(MapKey::W->value)),
        opts.i32(std::get<std::string>(MapKey::H->value)));
    }

    /* Display make function */
    FUNC_IMPL(MakeDisplay,
              SIG((FN_ARGS((&Roo::Type::MAP)), EXEC_DISPATCH(&MakeDisplay::exec_make))))

    Roo::MapSchema display_schema(
      {{std::get<std::string>(MapKey::RESOLUTION->value), &HostType::RESOLUTION}},
      {{std::get<std::string>(MapKey::ALIGN->value), &Roo::Type::KEYWORD},
       {std::get<std::string>(MapKey::SCALING->value), &Roo::Type::KEYWORD},
       {std::get<std::string>(MapKey::BACKGROUND->value), &HostType::COLOR}});

    EXEC_BODY(MakeDisplay, exec_make)
    {
      try
      {
        auto opts = display_schema.bind(ctx, *args[0]);

        auto align = Display::Alignment::NONE;
        auto scaling = Display::Scaling::NONE;

        if (opts.contains(std::get<std::string>(MapKey::ALIGN->value)))
        {
          const std::string align_str =
            opts.str(std::get<std::string>(MapKey::ALIGN->value));
          if (align_str == "align/center")
          {
            align = Display::Alignment::CENTER;
          }
        }

        if (opts.contains(std::get<std::string>(MapKey::SCALING->value)))
        {
          const std::string scale_str =
            opts.str(std::get<std::string>(MapKey::SCALING->value));
          if (scale_str == "scaling/stretch")
          {
            scaling = Display::Scaling::STRETCH;
          }
          else if (scale_str == "scaling/fit")
          {
            scaling = Display::Scaling::FIT;
          }
        }

        Color color =
          opts.obj<Color>(std::get<std::string>(MapKey::BACKGROUND->value), Color{0, 0, 0});

        return DisplayAdapter::make_unique(
          opts.obj<Resolution>(std::get<std::string>(MapKey::RESOLUTION->value)),
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
                        (FN_ARGS((&Roo::Type::KEYWORD)),
                         EXEC_DISPATCH(&MakeResolution::exec_make_resolution)),
                        (FN_ARGS((&Roo::Type::MAP)),
                         EXEC_DISPATCH(&MakeResolution::exec_make_resolution))));

    EXEC_BODY(MakeResolution, exec_make_resolution)
    {
      if (Roo::Type::KEYWORD.is_type_of(*args[0]))
      {
        const std::string& res_type = args[0]->str();
        if (res_type == "auto")
        {
          return ResolutionAdapter::make_unique(Resolution::Mode::AUTO);
        }
        throw Roo::TypeError("Invalid resolution specifier: " + args[0]->to_string());
      }
      else if (Roo::Type::MAP.is_type_of(*args[0]))
      {
        auto scale_val = Roo::Dict::get_property(args[0], Roo::keyword("scale"));
        if (scale_val && scale_val->type == Roo::Value::Type::NUMBER)
        {
          int ps = scale_val->num().get_int();
          return ResolutionAdapter::make_unique(Resolution::Mode::AUTO, ps);
        }

        Roo::CoercionResult cresult = HostType::DIMENSION.coerce(ctx, args[0]);
        if (cresult.success)
        {
          return ResolutionAdapter::make_unique(Resolution::Mode::FIXED,
                                                Roo::obj<Dimension>(*cresult.result));
        }
      }

      throw Roo::TypeError("Could not construct Resolution from: " + args[0]->to_string());
    }

    /* PushModeBangFunction - push-mode! */
    FUNC_IMPL(
      PushModeBangFunction,
      MULTI_SIG((FN_ARGS((&Roo::Type::SYMBOL_VALUE)),
                 EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode)),
                (FN_ARGS((&Roo::Type::SYMBOL_VALUE), (&Roo::Type::ANY)),
                 EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode)),
                (FN_ARGS((&Roo::Type::SYMBOL_VALUE), (&Roo::Type::ANY), (&Roo::Type::MAP)),
                 EXEC_DISPATCH(&PushModeBangFunction::exec_push_mode))));

    EXEC_BODY(PushModeBangFunction, exec_push_mode)
    {
      auto message_queue = ctx.lookup(ID__PIXILS__MODE_STACK_MESSAGES);

      Roo::append(*message_queue,
                  Roo::map({Roo::keyword(std::get<std::string>(MapKey::TYPE->value)),
                            Roo::keyword(std::get<std::string>(MapKey::PUSH->value)),
                            Roo::keyword(std::get<std::string>(MapKey::MODE->value)),
                            args.front(),
                            Roo::keyword(std::get<std::string>(MapKey::STATE->value)),
                            args.size() > 1 ? args[1] : Roo::Constant::NIL,
                            Roo::keyword("overrides"),
                            args.size() > 2 ? args[2] : Roo::Constant::NIL}));

      return args[0];
    }

    /* PopModeBangFunction - pop-mode! */
    FUNC_IMPL(PopModeBangFunction,
              MULTI_SIG((NO_ARGS, EXEC_DISPATCH(&PopModeBangFunction::exec_pop_mode)),
                        (FN_ARGS((&Roo::Type::ANY)),
                         EXEC_DISPATCH(&PopModeBangFunction::exec_pop_mode))));

    EXEC_BODY(PopModeBangFunction, exec_pop_mode)
    {
      auto message_queue = ctx.lookup(ID__PIXILS__MODE_STACK_MESSAGES);

      Roo::append(*message_queue,
                  Roo::map({
                    Roo::keyword(std::get<std::string>(MapKey::TYPE->value)),
                    Roo::keyword(std::get<std::string>(MapKey::POP->value)),
                    Roo::keyword("payload"),
                    args.empty() ? Roo::Constant::NIL : args[0],
                  }));

      return Roo::Constant::NIL;
    }

    /* QuitBangFunction - quit! */
    FUNC_IMPL(QuitBangFunction, SIG((NO_ARGS, EXEC_DISPATCH(&QuitBangFunction::exec_quit))));

    EXEC_BODY(QuitBangFunction, exec_quit)
    {
      auto message_queue = ctx.lookup(ID__PIXILS__MODE_STACK_MESSAGES);

      Roo::append(*message_queue,
                  Roo::map({
                    Roo::keyword(std::get<std::string>(MapKey::TYPE->value)),
                    Roo::keyword(std::get<std::string>(MapKey::QUIT->value)),
                  }));

      return Roo::Constant::NIL;
    }

    /* SetThemeBangFunction - set-theme! */
    FUNC_IMPL(SetThemeBangFunction,
              MULTI_SIG((FN_ARGS((&Roo::Type::ANY)),
                         EXEC_DISPATCH(&SetThemeBangFunction::exec_set_theme)),
                        (FN_ARGS((&Roo::Type::ANY), (&Roo::Type::ANY)),
                         EXEC_DISPATCH(&SetThemeBangFunction::exec_set_theme))));

    EXEC_BODY(SetThemeBangFunction, exec_set_theme)
    {
      auto message_queue = ctx.lookup(ID__PIXILS__MODE_STACK_MESSAGES);

      Roo::append(*message_queue,
                  Roo::map({
                    Roo::keyword(std::get<std::string>(MapKey::TYPE->value)),
                    Roo::keyword("theme"),
                    Roo::keyword(std::get<std::string>(MapKey::THEME->value)),
                    args[0],
                    Roo::keyword(std::get<std::string>(MapKey::THEME_VARIANT->value)),
                    args.size() > 1 ? args[1] : Roo::Constant::NIL,
                  }));

      return args[0];
    }

    /* ThemeVarFunction - var */
    FUNC_IMPL(ThemeVarFunction,
              SIG((FN_ARGS((&Roo::Type::ANY)),
                   EXEC_DISPATCH(&ThemeVarFunction::exec_theme_var))));

    EXEC_BODY(ThemeVarFunction, exec_theme_var)
    {
      auto key = args[0];
      if (key->type != Roo::Value::Type::KEYWORD && key->type != Roo::Value::Type::SYMBOL)
      {
        throw Roo::TypeError("var expects a keyword or symbol");
      }
      return Roo::map({Roo::keyword("__pixils-theme-var"), key});
    }

    /* WarpMouseBangFunction - warp-mouse! */
    FUNC_IMPL(WarpMouseBangFunction,
              MULTI_SIG((FN_ARGS((&HostType::HOOK_CONTEXT), (&HostType::POINT)),
                         EXEC_DISPATCH(&WarpMouseBangFunction::exec_warp_mouse)),
                        (FN_ARGS((&HostType::POINT)),
                         EXEC_DISPATCH(&WarpMouseBangFunction::exec_warp_mouse))));

    EXEC_BODY(WarpMouseBangFunction, exec_warp_mouse)
    {
      Point point;
      if (args.size() == 2)
      {
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[0]);
        point = Roo::obj<Point>(*args[1]);
        if (hook_ctx.render)
        {
          const_cast<RenderContext*>(hook_ctx.render)->warp_mouse_to_buffer_point(point);
        }
        if (hook_ctx.events)
        {
          hook_ctx.events->do_mouse_motion(point.round_x(), point.round_y());
        }
      }
      else
      {
        point = Roo::obj<Point>(*args[0]);
        RenderContext& rc = Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
        rc.warp_mouse_to_buffer_point(point);
      }

      return PointAdapter::make_unique(point.x, point.y);
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

  Roo::sptr_val ModeCompositionAdapter::get_render() const
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
    return Roo::number(object->get_object().render->pixel_size);
  }

  NOBJ_PROP_GET(HookContextAdapter, buffer_dim)
  {
    const Dimension& dim = object->get_object().render->buffer_dim;
    return DimensionAdapter::make_unique(dim.w, dim.h);
  }

  NOBJ_PROP_GET(HookContextAdapter, available_width)
  {
    auto available = object->get_object().available_width;
    return available ? Roo::number(*available) : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(HookContextAdapter, available_height)
  {
    auto available = object->get_object().available_height;
    return available ? Roo::number(*available) : Roo::Constant::NIL;
  }

  NOBJ_PROP_GET(HookContextAdapter, view)
  {
    auto view_ptr = object->get_object().current_view;
    if (!view_ptr) return Roo::Constant::NIL;
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
    Roo::sptr_val_v buttons;
    for (UI::MouseButton btn : object->get_object().pressed)
      buttons.push_back(Roo::keyword(UI::mouse_button_name(btn)));
    return Roo::vector(buttons);
  }

  /* ViewAdapter */
  NATIVE_ADAPTER_IMPL(ViewAdapter,
                      Runtime::View,
                      &HostType::VIEW,
                      (id),
                      (state),
                      (bounds),
                      ("external-bounds", external_bounds),
                      ("visual-bounds", visual_bounds),
                      ("visual-scale", visual_scale),
                      (interaction),
                      (style),
                      ("effective-style", effective_style),
                      ("on-click", on_click),
                      ("on-double-click", on_double_click),
                      ("on-mouse-down", on_mouse_down),
                      ("on-mouse-up", on_mouse_up));

  NOBJ_PROP_GET__FIELD(ViewAdapter, id);
  NOBJ_PROP_GET(ViewAdapter, state)
  {
    return object->get_object().state;
  }
  NOBJ_PROP_GET(ViewAdapter, bounds)
  {
    const Rect& b = object->get_object().bounds;
    return RectAdapter::make_unique(b.x, b.y, b.w, b.h);
  }

  NOBJ_PROP_GET(ViewAdapter, external_bounds)
  {
    Rect b = best_known_external_bounds(object->get_object());
    return RectAdapter::make_unique(b.x, b.y, b.w, b.h);
  }

  NOBJ_PROP_GET(ViewAdapter, visual_bounds)
  {
    const Rect& b = object->get_object().visual_bounds;
    return RectAdapter::make_unique(b.x, b.y, b.w, b.h);
  }

  NOBJ_PROP_GET(ViewAdapter, visual_scale)
  {
    return Roo::number(object->get_object().visual_scale);
  }

  NOBJ_PROP_GET(ViewAdapter, interaction)
  {
    return InteractionStateAdapter::make_unique(object->get_object().interaction);
  }

  NOBJ_PROP_GET(ViewAdapter, style)
  {
    const Runtime::View& v = object->get_object();
    if (!v.mode || !v.mode->style.has_value()) return Roo::Constant::NIL;
    return StyleAdapter::make_ref(*v.mode->style);
  }

  NOBJ_PROP_GET(ViewAdapter, effective_style)
  {
    return StyleAdapter::make_ref(object->get_object().effective_style);
  }

  NOBJ_PROP_GET(ViewAdapter, on_click)
  {
    const Runtime::View& v = object->get_object();
    return v.owned_mode && *v.owned_mode->on_click != *Roo::Constant::NIL
             ? v.owned_mode->on_click
             : v.mode->on_click;
  }

  NOBJ_PROP_GET(ViewAdapter, on_double_click)
  {
    const Runtime::View& v = object->get_object();
    return v.owned_mode && *v.owned_mode->on_double_click != *Roo::Constant::NIL
             ? v.owned_mode->on_double_click
             : v.mode->on_double_click;
  }

  NOBJ_PROP_GET(ViewAdapter, on_mouse_up)
  {
    const Runtime::View& v = object->get_object();
    return v.owned_mode && *v.owned_mode->on_mouse_up != *Roo::Constant::NIL
             ? v.owned_mode->on_mouse_up
             : v.mode->on_mouse_up;
  }

  NOBJ_PROP_GET(ViewAdapter, on_mouse_down)
  {
    const Runtime::View& v = object->get_object();
    return v.owned_mode && *v.owned_mode->on_mouse_down != *Roo::Constant::NIL
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
                      ("theme-variant", theme_variant),
                      ("target-frame-rate", target_frame_rate));

  NOBJ_PROP_GET__METHOD(ProgramAdapter, name);
  NOBJ_PROP_GET_SET_ADAPTER__FIELD(ProgramAdapter, display, DisplayAdapter);

  NOBJ_PROP_GET(ProgramAdapter, initial_mode)
  {
    if (get_object().initial_mode == "")
    {
      return Roo::Constant::NIL;
    }
    return Roo::symbol(get_object().initial_mode);
  };

  NOBJ_PROP_GET(ProgramAdapter, theme)
  {
    if (!get_object().theme) return Roo::Constant::NIL;
    const auto& theme_names = *get_object().theme;
    if (theme_names.size() == 1)
    {
      return Roo::symbol(theme_names[0]);
    }

    std::vector<Roo::sptr_val> values;
    values.reserve(theme_names.size());
    for (const auto& theme_name : theme_names)
    {
      values.push_back(Roo::symbol(theme_name));
    }
    return Roo::vector(values);
  }

  NOBJ_PROP_GET(ProgramAdapter, theme_variant)
  {
    if (!get_object().theme_variant) return Roo::Constant::NIL;
    return Roo::keyword(*get_object().theme_variant);
  }

  NOBJ_PROP_GET__FIELD(ProgramAdapter, target_frame_rate);

  /* DisplayAdapter */
  NATIVE_ADAPTER_IMPL(DisplayAdapter, Display, &HostType::DISPLAY, (resolution));

  NOBJ_PROP_GET_SET_ADAPTER__FIELD(DisplayAdapter, resolution, ResolutionAdapter);

  /* ResolutionAdapter */
  NATIVE_ADAPTER_IMPL(ResolutionAdapter, Resolution, &HostType::RESOLUTION, (dimension));

  NOBJ_PROP_GET_ADAPTER__FIELD(ResolutionAdapter, dimension, DimensionAdapter);

  PixilsNamespace::PixilsNamespace(const RenderContext& render_context)
    : Roo::Namespace(NS_PIXILS)
  {
    values.emplace("mode-stack", Roo::vector({}));
    values.emplace("mode-stack-messages", Roo::vector({}));
    values.emplace("modes", Roo::map({}));
    values.emplace("themes", Roo::map({}));
    values.emplace("defpointer", Macro::DefPointerForm::make());
    values.emplace("defbundle", Macro::DefBundleForm::make());
    values.emplace("defbundle-dynamic", Macro::DefBundleDynamicForm::make());
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
    values.emplace("programs", Roo::map({}));
    values.emplace("pop-mode!", Function::PopModeBangFunction::make());
    values.emplace(FN__PUSH_MODE_BANG, Function::PushModeBangFunction::make());
    values.emplace(FN__QUIT_BANG, Function::QuitBangFunction::make());
    values.emplace(FN__SET_THEME_BANG, Function::SetThemeBangFunction::make());
    values.emplace(FN__WARP_MOUSE_BANG, Function::WarpMouseBangFunction::make());
    values.emplace("var", Function::ThemeVarFunction::make());
  }

} // namespace Pixils::Script
