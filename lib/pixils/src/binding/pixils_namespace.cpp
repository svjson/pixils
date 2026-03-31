
#include "pixils/binding/color_namespace.h"
#include "pixils/binding/resource_namespace.h"
#include "pixils/display.h"
#include "pixils/runtime/mode.h"
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>

#include <SDL2/SDL_render.h>
#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(ALIGN, "align");
    SHKEY(BACKGROUND, "background");
    SHKEY(BLOCK, "block");
    SHKEY(BUFFER_SIZE, "buffer-size");
    SHKEY(COMPOSE, "compose");
    SHKEY(DIMENSION, "dimension");
    SHKEY(DISPLAY, "display");
    SHKEY(H, "h");
    SHKEY(HELD_KEYS, "held-keys");
    SHKEY(INIT, "init");
    SHKEY(INITIAL_MODE, "initial-mode");
    SHKEY(KEY_DOWN, "key-down");
    SHKEY(MODE, "mode");
    SHKEY(NAME, "name");
    SHKEY(PIXEL_SIZE, "pixel-size");
    SHKEY(PASS, "pass");
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

  namespace Macro
  {
    /* DefProgramMacro - defprogram */
    MACRO_IMPL(DefProgramMacro,
               SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                            (&Lisple::Type::MAP)),
                    EXEC_DISPATCH(&DefProgramMacro::def_program))));

    Lisple::MapSchema program_schema({},
                                     {{"display", &HostType::DISPLAY},
                                      {"initial-mode", &Lisple::Type::SYMBOL_VALUE}});

    MACRO_BODY(DefProgramMacro, def_program)
    {
      Lisple::sptr_rtval_v rargs = {Lisple::to_rt_value(args[0]),
                                    Lisple::to_rt_value(args[1])};
      auto name = rargs[0]->str();
      auto opts = program_schema.bind(ctx, *rargs[1]);

      auto programs = ctx.lookup_value(ID__PIXILS__PROGRAMS);
      auto initial_mode = opts.str(MapKey::INITIAL_MODE->value, "");

      Display display = opts.contains(MapKey::DISPLAY->value)
                          ? opts.obj<Display>(MapKey::DISPLAY->value)
                          : Display(Resolution(Resolution::Mode::AUTO, {0, 0}),
                                    Display::Alignment::NONE,
                                    Display::Scaling::NONE,
                                    Color(0, 0, 0));

      auto program =
        Lisple::RTValue::object(ProgramAdapter::make<Program>(name, display, initial_mode));

      Lisple::Dict::set_property(programs, Lisple::RTValue::symbol(name), program);

      return Lisple::NIL;
    }

    /* DefModeMacro - defmode */
    MACRO_IMPL(DefModeMacro,
               SIG((FN_ARGS((&Lisple::Type::WORD, &Lisple::Eval::LITERAL),
                            (&HostType::MODE, &Lisple::Eval::LITERAL)),
                    EXEC_DISPATCH(&DefModeMacro::declare_mode))));

    MACRO_BODY(DefModeMacro, declare_mode)
    {
      Lisple::Map& modes = ctx.lookup(ID__PIXILS__MODES)->as<Lisple::Map>();
      args.back()->as<ModeAdapter>().get_object().name = args.front()->to_string();
      modes.set_property(args.front(), args.back());
      return Lisple::NIL;
    }
  } // namespace Macro

  namespace Function
  {
    /* Mode make function */
    FUNC_IMPL(MakeMode, SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeMode::make))))

    ArgCollector mode_collector(FN__PIXILS__MAKE_MODE,
                                {},
                                {{*MapKey::INIT, &Lisple::Type::ANY},
                                 {*MapKey::UPDATE, &Lisple::Type::ANY},
                                 {*MapKey::RENDER, &Lisple::Type::ANY},
                                 {*MapKey::COMPOSE, &HostType::MODE_COMPOSITION},
                                 {*MapKey::RESOURCES, &HostType::RESOURCE_DEPENDENCIES}});

    FUNC_BODY(MakeMode, make)
    {
      str_key_map_t keys = mode_collector.collect_keys(ctx, *args.front());

      Lisple::Map& proto = args.front()->as<Lisple::Map>();

      auto resources = ArgCollector::optional_host_object<Runtime::ResourceDependencies>(
        ctx,
        keys,
        *MapKey::RESOURCES,
        &HostType::RESOURCE_DEPENDENCIES);

      auto composition = ArgCollector::optional_host_object<Runtime::ModeComposition>(
        ctx,
        keys,
        *MapKey::COMPOSE,
        &HostType::MODE_COMPOSITION);

      Runtime::Mode mode{.name = proto.get_sptr_property(*MapKey::NAME)->to_string(),
                         .resources = {},
                         .init = proto.get_sptr_property(*MapKey::INIT),
                         .update = proto.get_sptr_property(*MapKey::UPDATE),
                         .render = proto.get_sptr_property(*MapKey::RENDER),
                         .composition = {}};

      if (resources.has_value()) mode.resources = *resources;
      if (composition.has_value()) mode.composition = *composition;

      return ModeAdapter::make<Runtime::Mode>(mode);
    }

    /* ModeComposition make function */
    FUNC_IMPL(MakeModeComposition,
              SIG((FN_ARGS((&Lisple::Type::MAP)),
                   EXEC_DISPATCH(&MakeModeComposition::exec_make))));

    EXEC_BODY(MakeModeComposition, exec_make)
    {
      static Lisple::MapSchema mode_compose_schema({}, {{"render", &Lisple::Type::KEY}});

      auto opts = mode_compose_schema.bind(ctx, *args[0]);

      Runtime::ModeComposition composition{opts.str("render", "block") == "pass"};

      return Lisple::RTValue::object(std::make_shared<ModeCompositionAdapter>(
        std::make_unique<Runtime::ModeComposition>(composition)));
    }

    /* Dimension make function */
    FUNC_IMPL(MakeDimension,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeDimension::exec_make))))

    Lisple::MapSchema dimension_schema({{MapKey::W->value, &Lisple::Type::NUMBER},
                                        {MapKey::H->value, &Lisple::Type::NUMBER}});

    EXEC_BODY(MakeDimension, exec_make)
    {
      auto opts = dimension_schema.bind(ctx, *args[0]);
      return Lisple::RTValue::object(
        DimensionAdapter::make<Dimension>(opts.i32(MapKey::W->value),
                                          opts.i32(MapKey::H->value)));
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

        return Lisple::RTValue::object(
          DisplayAdapter::make<Display>(opts.obj<Resolution>(MapKey::RESOLUTION->value),
                                        align,
                                        scaling,
                                        color));
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
          return Lisple::RTValue::object(
            ResolutionAdapter::make<Resolution>(Resolution::Mode::AUTO));
        }
        throw Lisple::TypeError("Invalid resolution specifier: " + args[0]->to_string());
      }
      else if (Lisple::Type::MAP.is_type_of(*args[0]))
      {
        Lisple::CoercionResult<Lisple::RTValue> cresult =
          HostType::DIMENSION.coerce(ctx, args[0]);
        if (cresult.success)
        {
          return Lisple::RTValue::object(
            ResolutionAdapter::make<Resolution>(Resolution::Mode::FIXED,
                                                Lisple::obj<Dimension>(*cresult.result)));
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
  HOST_ADAPTER_IMPL(ModeAdapter,
                    Runtime::Mode,
                    &HostType::MODE,
                    ({K_GET(ModeAdapter, MapKey::INIT, init),
                      K_GET(ModeAdapter, MapKey::UPDATE, update),
                      K_GET(ModeAdapter, MapKey::RENDER, render)}))

  Lisple::sptr_sobject ModeAdapter::get_init() const
  {
    return this->get_object().init;
  }

  Lisple::sptr_sobject ModeAdapter::get_update() const
  {
    return this->get_object().update;
  }

  Lisple::sptr_sobject ModeAdapter::get_render() const
  {
    return this->get_object().render;
  }

  /* ModeCompositionAdapter */
  HOST_ADAPTER_IMPL(ModeCompositionAdapter,
                    Runtime::ModeComposition,
                    &HostType::MODE_COMPOSITION,
                    ({K_GET(ModeCompositionAdapter, MapKey::RENDER, render)}));

  Lisple::sptr_sobject ModeCompositionAdapter::get_render() const
  {
    return this->get_object().render ? MapKey::PASS : MapKey::BLOCK;
  }

  /* DimensionAdapter */
  HOST_ADAPTER_IMPL(DimensionAdapter,
                    Dimension,
                    &HostType::DIMENSION,
                    ({K_GET_SET(DimensionAdapter, MapKey::W, w),
                      K_GET_SET(DimensionAdapter, MapKey::H, h)}));

  ADAPTER_PROP_GET_SET__FIELD(DimensionAdapter, w, Lisple::Number, w);
  ADAPTER_PROP_GET_SET__FIELD(DimensionAdapter, h, Lisple::Number, h);

  /* FrameEventsAdapter */
  HOST_ADAPTER_IMPL(FrameEventsAdapter,
                    FrameEvents,
                    &HostType::FRAME_EVENTS,
                    ({K_GET(FrameEventsAdapter, MapKey::KEY_DOWN, key_down),
                      K_GET(FrameEventsAdapter, MapKey::HELD_KEYS, held_keys)}))

  ADAPTER_PROP_GET(FrameEventsAdapter, key_down)
  {
    return object->get_object().key_down;
  }

  ADAPTER_PROP_GET(FrameEventsAdapter, held_keys)
  {
    return object->get_object().held_keys;
  }

  /* RenderContextAdapter */
  HOST_ADAPTER_IMPL(RenderContextAdapter,
                    RenderContext,
                    &HostType::RENDER_CONTEXT,
                    ({K_GET(RenderContextAdapter, MapKey::PIXEL_SIZE, pixel_size),
                      K_GET(RenderContextAdapter, MapKey::BUFFER_SIZE, buffer_size)}));

  ADAPTER_PROP_GET__FIELD(RenderContextAdapter, pixel_size, Lisple::Number);
  ADAPTER_PROP_GET__FIELD(RenderContextAdapter, buffer_size, DimensionAdapter, buffer_dim);

  /* ProgramAdapter */
  HOST_ADAPTER_IMPL(ProgramAdapter,
                    Program,
                    &HostType::PROGRAM,
                    ({K_GET(ProgramAdapter, MapKey::NAME, name),
                      K_GET(ProgramAdapter, MapKey::INITIAL_MODE, initial_mode),
                      K_GET_SET(ProgramAdapter, MapKey::DISPLAY, display)}));

  ADAPTER_PROP_GET__METHOD(ProgramAdapter, name, Lisple::String, get_name);
  ADAPTER_PROP_GET_SET_HOST_OBJECT__FIELD(ProgramAdapter, display, DisplayAdapter);
  Lisple::sptr_sobject ProgramAdapter::get_initial_mode() const
  {
    if (get_self_object().initial_mode == "")
    {
      return Lisple::NIL;
    }
    return std ::make_shared<Lisple ::Word>(get_self_object().initial_mode);
  };

  /* DisplayAdapter */
  HOST_ADAPTER_IMPL(DisplayAdapter,
                    Display,
                    &HostType::DISPLAY,
                    ({K_GET_SET(DisplayAdapter, MapKey::RESOLUTION, resolution)}));

  ADAPTER_PROP_GET_SET_HOST_OBJECT__FIELD(DisplayAdapter, resolution, ResolutionAdapter);

  /* ResolutionAdapter */
  HOST_ADAPTER_IMPL(ResolutionAdapter,
                    Resolution,
                    &HostType::RESOLUTION,
                    ({K_GET(ResolutionAdapter, MapKey::DIMENSION, dimension)}));

  ADAPTER_PROP_GET__FIELD(ResolutionAdapter, dimension, DimensionAdapter);

  PixilsNamespace::PixilsNamespace(const RenderContext& render_context)
    : Lisple::Namespace(NS_PIXILS)
  {
    values.emplace("mode-stack", Lisple::RTValue::vector({}));
    values.emplace("mode-stack-messages", Lisple::RTValue::vector({}));
    objects.emplace("modes", Lisple::Map::make({}));
    objects.emplace("defmode", std::make_shared<Macro::DefModeMacro>());
    objects.emplace("defprogram", std::make_shared<Macro::DefProgramMacro>());
    objects.emplace("make-dimension", std::make_shared<Function::MakeDimension>());
    objects.emplace("make-display", std::make_shared<Function::MakeDisplay>());
    objects.emplace("make-mode", std::make_shared<Function::MakeMode>());
    values.emplace("make-mode-composition", Function::MakeModeComposition::make());
    objects.emplace("make-resolution", std::make_shared<Function::MakeResolution>());
    objects.emplace("render-context", RenderContextAdapter::make_ref(render_context));
    values.emplace("programs", Lisple::RTValue::map({}));
    objects.emplace("pop-mode!", std::make_shared<Function::PopModeBangFunction>());
    objects.emplace("push-mode!", std::make_shared<Function::PushModeBangFunction>());
  }

} // namespace Pixils::Script
