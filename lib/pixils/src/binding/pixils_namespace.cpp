
#include "pixils/binding/color_namespace.h"
#include "pixils/binding/resource_namespace.h"
#include "pixils/binding/style_namespace.h"
#include "pixils/display.h"
#include "pixils/runtime/mode.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>

#include <SDL2/SDL_render.h>
#include <lisple/exec.h>
#include <lisple/form.h>
#include <lisple/host.h>
#include <lisple/host/accessor.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/exec_node.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>
#include <unordered_map>

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
    SHKEY(RENDER, "render");
    SHKEY(RESOLUTION, "resolution");
    SHKEY(RESOURCES, "resources");
    SHKEY(SCALING, "scaling");
    SHKEY(STATE, "state");
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
    /* DefProgramForm - defprogram */
    SPECIAL_FORM_IMPL(DefProgramForm,
                      SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                                   (&Lisple::Type::MAP)),
                           EXEC_DISPATCH(&DefProgramForm::inv_def_program,
                                         &DefProgramForm::execnode_def_program))));

    Lisple::MapSchema program_schema({},
                                     {{"display", &HostType::DISPLAY},
                                      {"initial-mode", &Lisple::Type::SYMBOL_VALUE},
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

      if (opts.str("pointer", "") == "off")
      {
        Lisple::obj<Program>(*program).pointer_visible = false;
      }

      Lisple::Dict::set_property(programs, Lisple::RTValue::symbol(name), program);

      return std::make_unique<Lisple::ExecNode>(Lisple::Constant::NIL);
    }

    EXECNODE_BODY(DefProgramForm, execnode_def_program)
    {
      throw Lisple::LispleException("defmode is lower-only");
    }

    MACRO_BODY(DefProgramForm, inv_def_program)
    {
      return Lisple::NIL;
    }

    /* DefModeForm - defmode */
    SPECIAL_FORM_IMPL(DefModeForm,
                      SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                                   (&HostType::MODE, &Lisple::Eval::LITERAL)),
                           EXEC_DISPATCH(&DefModeForm::inv_declare_mode,
                                         &DefModeForm::execnode_declare_mode))));

    SFORM_LOWER_IMPL(DefModeForm)
    {
      auto modes = ctx.ctx->lookup_value(ID__PIXILS__MODES);
      auto name_expr = Lisple::exec(*ctx.ctx, *lower_literal(ast_node->get_children()[1]));
      auto name_str = Lisple::RTValue::string(name_expr->str());

      auto mode_expr = Lisple::exec(*ctx.ctx, *lower_literal(ast_node->get_children()[2]));
      Lisple::Dict::set_property(mode_expr, MapKey::NAME, name_str);
      auto mode_coercion = HostType::MODE.coerce(*ctx.ctx, mode_expr);
      if (!mode_coercion.success)
      {
        throw Lisple::TypeError("Invalid mode declaration: " + mode_expr->to_string());
      }

      Lisple::Dict::set_property(modes, name_expr, mode_coercion.result);

      return std::make_unique<Lisple::ExecNode>(Lisple::Constant::NIL);
    }

    MACRO_BODY(DefModeForm, inv_declare_mode)
    {
      Lisple::Map& modes = ctx.lookup(ID__PIXILS__MODES)->as<Lisple::Map>();
      args.back()->as<ModeAdapter>().get_object().name = args.front()->to_string();
      modes.set_property(args.front(), args.back());
      return Lisple::NIL;
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

    Lisple::sptr_rtval eval_hook(Lisple::Context& ctx, const Lisple::sptr_rtval& hook_value)
    {
      if (hook_value->type == Lisple::RTValue::Type::LIST)
      {
        return ctx.eval(hook_value->to_string());
      }
      return hook_value;
    }

    EXEC_BODY(MakeMode, exec_make)
    {
      static Lisple::MapSchema mode_schema({},
                                           {{"name", &Lisple::Type::STRING},
                                            {"init", &Lisple::Type::ANY},
                                            {"update", &Lisple::Type::ANY},
                                            {"render", &Lisple::Type::ANY},
                                            {"compose", &HostType::MODE_COMPOSITION},
                                            {"resources", &HostType::RESOURCE_DEPENDENCIES},
                                            {"layout", &Lisple::Type::KEY},
                                            {"style", &HostType::STYLE},
                                            {"children", &Lisple::Type::ANY}});

      auto opts = mode_schema.bind(ctx, *args[0]);

      auto resources = opts.optional_obj<Runtime::ResourceDependencies>("resources");
      auto composition = opts.optional_obj<Runtime::ModeComposition>("compose");

      auto init_expr = eval_hook(ctx, opts.val("init"));
      auto update_expr = eval_hook(ctx, opts.val("update"));
      auto render_expr = eval_hook(ctx, opts.val("render"));

      Runtime::Mode mode{.name = opts.str("name", ""),
                         .resources = {},
                         .init = init_expr,
                         .update = update_expr,
                         .render = render_expr,
                         .composition = {},
                         .children = {}};

      if (resources.has_value()) mode.resources = *resources;
      if (composition.has_value()) mode.composition = *composition;

      if (opts.contains("layout"))
      {
        auto layout_val = opts.val("layout");
        auto [ns, name] = layout_val->qual();
        if (name == "row") mode.layout_direction = Runtime::LayoutDirection::ROW;
      }

      mode.style = opts.optional_obj<UI::Style>("style");

      auto children_val = opts.val("children");
      if (children_val->type != Lisple::RTValue::Type::NIL)
      {
        static Lisple::MapSchema child_schema({{"mode", &Lisple::Type::SYMBOL}},
                                              {{"id", &Lisple::Type::ANY},
                                               {"state", &Lisple::Type::ANY},
                                               {"width", &Lisple::Type::NUMBER},
                                               {"height", &Lisple::Type::NUMBER}});
        std::unordered_map<std::string, int> mode_name_counts;

        // FIXME: This is a hack because of the non-expr eval of the defmode map.
        // Perhaps there isn't actually any need for that?
        if (children_val->type == Lisple::RTValue::Type::LIST)
        {
          children_val = ctx.eval(children_val);
        }

        size_t n = Lisple::count(*children_val);
        for (size_t i = 0; i < n; i++)
        {
          auto child_entry = Lisple::get_child(*children_val, i);
          auto mode_sym = Lisple::Dict::get_property(*child_entry, "mode");
          auto child_opts = child_schema.bind(ctx, *child_entry);

          Runtime::ChildSlot slot;
          slot.mode_name = mode_sym->str();

          if (child_opts.contains("id"))
          {
            slot.id = child_opts.val("id")->str();
          }
          else
          {
            int idx = mode_name_counts[slot.mode_name]++;
            slot.id = slot.mode_name + "-" + std::to_string(idx);
          }

          slot.initial_state = child_opts.val("state");

          if (child_opts.contains("width"))
            slot.width = Runtime::DimensionConstraint::fixed(child_opts.i32("width"));
          if (child_opts.contains("height"))
            slot.height = Runtime::DimensionConstraint::fixed(child_opts.i32("height"));

          mode.children.push_back(slot);
        }
      }

      return ModeAdapter::make_unique(mode);
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
                              args.size() > 1 ? args[1] : Lisple::Constant::NIL}));

      return args[0];
    }

    /* PopModeBangFunction - pop-mode! */
    FUNC_IMPL(PopModeBangFunction,
              SIG((NO_ARGS, EXEC_DISPATCH(&PopModeBangFunction::exec_pop_mode))));

    EXEC_BODY(PopModeBangFunction, exec_pop_mode)
    {
      auto message_queue = ctx.lookup_value(ID__PIXILS__MODE_STACK_MESSAGES);

      Lisple::append(*message_queue,
                     Lisple::RTValue::map({
                       Lisple::RTValue::keyword(MapKey::TYPE->value),
                       Lisple::RTValue::keyword(MapKey::POP->value),
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

  Lisple::sptr_rtval ModeAdapter::get_init() const
  {
    return this->get_object().init;
  }

  Lisple::sptr_rtval ModeAdapter::get_update() const
  {
    return this->get_object().update;
  }

  Lisple::sptr_rtval ModeAdapter::get_render() const
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
                      ("mouse-held", mouse_held),
                      ("pixel-size", pixel_size),
                      ("buffer-size", buffer_dim));

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
                      (display));

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
    values.emplace("defmode", Macro::DefModeForm::make());
    values.emplace("defcomponent", Macro::DefModeForm::make());
    values.emplace("defprogram", Macro::DefProgramForm::make());
    values.emplace("make-dimension", Function::MakeDimension::make());
    values.emplace("make-display", Function::MakeDisplay::make());
    values.emplace("make-mode", Function::MakeMode::make());
    values.emplace("make-mode-composition", Function::MakeModeComposition::make());
    values.emplace("make-resolution", Function::MakeResolution::make());
    values.emplace("render-context", RenderContextAdapter::make_ref(render_context));
    values.emplace("programs", Lisple::RTValue::map({}));
    values.emplace("pop-mode!", Function::PopModeBangFunction::make());
    values.emplace("push-mode!", Function::PushModeBangFunction::make());
  }

} // namespace Pixils::Script
