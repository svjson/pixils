
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>

#include <SDL2/SDL_render.h>
#include <lisple/exec.h>
#include <lisple/host.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    SHKEY(BUFFER_SIZE, "buffer-size");
    SHKEY(H, "h");
    SHKEY(HELD_KEYS, "held-keys");
    SHKEY(INIT, "init");
    SHKEY(KEY_DOWN, "key-down");
    SHKEY(NAME, "name");
    SHKEY(PIXEL_SIZE, "pixel-size");
    SHKEY(RENDER, "render");
    SHKEY(UPDATE, "update");
    SHKEY(W, "w");
  } // namespace MapKey

  namespace Macro
  {
    /* DefModeMacro - defmode */
    MACRO_IMPL(DefModeMacro, SIG((FN_ARGS((&Lisple::Type::WORD, Lisple::NO_EVAL),
                                          (&HostType::MODE, Lisple::NO_EVAL)),
                                  EXEC_DISPATCH(&DefModeMacro::declare_mode))));

    MACRO_BODY(DefModeMacro, declare_mode)
    {
      Lisple::Map& modes = ctx.lookup(ID__PIXILS__MODES)->as<Lisple::Map>();
      modes.set_property(args.front(), args.back());
      return Lisple::NIL;
    }
  } // namespace Macro

  namespace Function
  {
    /* Mode make function */
    FUNC_IMPL(MakeMode, SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeMode::make))))

    FUNC_BODY(MakeMode, make)
    {
      Lisple::Map& proto = args.front()->as<Lisple::Map>();

      Runtime::Mode mode{.name = proto.get_sptr_property(*MapKey::NAME)->to_string(),
                         .init = proto.get_sptr_property(*MapKey::INIT),
                         .update = proto.get_sptr_property(*MapKey::UPDATE),
                         .render = proto.get_sptr_property(*MapKey::RENDER)};

      return ModeAdapter::make<Runtime::Mode>(mode);
    }

    /* Dimension make function */
    FUNC_IMPL(MakeDimension,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeDimension::make))))

    ArgCollector dimension_collector(FN__MAKE_DIMENSION,
                                     {{*MapKey::W, &Lisple::Type::NUMBER},
                                      {*MapKey::H, &Lisple::Type::NUMBER}});

    FUNC_BODY(MakeDimension, make)
    {
      str_key_map_t keys = dimension_collector.collect_keys(ctx, *args.front());
      return DimensionAdapter::make<Dimension>(ArgCollector::int_value(keys, *MapKey::W),
                                               ArgCollector::int_value(keys, *MapKey::H));
    }

    /* PushModeBangFunction - push-mode! */
    FUNC_IMPL(PushModeBangFunction, SIG((FN_ARGS((&Lisple::Type::SYMBOL)),
                                         EXEC_DISPATCH(&PushModeBangFunction::push_mode))));

    FUNC_BODY(PushModeBangFunction, push_mode)
    {
      Lisple::QSymbol& mode_name = args.front()->as<Lisple::QSymbol>();
      Lisple::Map& modes = ctx.lookup(ID__PIXILS__MODES)->as<Lisple::Map>();
      Lisple::Array& mode_stack = ctx.lookup(ID__PIXILS__MODE_STACK)->as<Lisple::Array>();

      Lisple::sptr_sobject new_mode = modes.get_sptr_property(Lisple::Word(mode_name.value));
      if (new_mode != Lisple::NIL)
      {
        mode_stack.append(new_mode);
      }
      return new_mode;
    }

    /* PopModeBangFunction - pop-mode! */
    FUNC_IMPL(PopModeBangFunction,
              SIG((NO_ARGS, EXEC_DISPATCH(&PopModeBangFunction::pop_mode))));

    FUNC_BODY(PopModeBangFunction, pop_mode)
    {
      Lisple::Array& mode_stack = ctx.lookup(ID__PIXILS__MODE_STACK)->as<Lisple::Array>();
      if (mode_stack.size() > 1)
      {
        Lisple::sptr_sobject old_mode = mode_stack.get_children().back();
        mode_stack.get_children().pop_back();
        return old_mode;
      }

      return Lisple::NIL;
    }

  } // namespace Function

  /* ModeAdapter */
  HOST_ADAPTER_IMPL(ModeAdapter, Runtime::Mode, &HostType::MODE,
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

  /* DimensionAdapter */
  HOST_ADAPTER_IMPL(DimensionAdapter, Dimension, &HostType::DIMENSION,
                    ({K_GET_SET(DimensionAdapter, MapKey::W, w),
                      K_GET_SET(DimensionAdapter, MapKey::H, h)}));

  ADAPTER_PROP_GET_SET__FIELD(DimensionAdapter, w, Lisple::Number, w);
  ADAPTER_PROP_GET_SET__FIELD(DimensionAdapter, h, Lisple::Number, h);

  /* FrameEventsAdapter */
  HOST_ADAPTER_IMPL(FrameEventsAdapter, FrameEvents, &HostType::FRAME_EVENTS,
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
  HOST_ADAPTER_IMPL(RenderContextAdapter, RenderContext, &HostType::RENDER_CONTEXT,
                    ({K_GET(RenderContextAdapter, MapKey::PIXEL_SIZE, pixel_size),
                      K_GET(RenderContextAdapter, MapKey::BUFFER_SIZE, buffer_size)}));

  ADAPTER_PROP_GET__FIELD(RenderContextAdapter, pixel_size, Lisple::Number);
  ADAPTER_PROP_GET__FIELD(RenderContextAdapter, buffer_size, DimensionAdapter, buffer_dim);

  PixilsNamespace::PixilsNamespace(const RenderContext& render_context)
      : Lisple::Namespace(NS_PIXILS)
  {
    objects.emplace("mode-stack", Lisple::Array::make({}));
    objects.emplace("modes", Lisple::Map::make({}));
    objects.emplace("defmode", std::make_shared<Macro::DefModeMacro>());
    objects.emplace("make-mode", std::make_shared<Function::MakeMode>());
    objects.emplace("render-context", RenderContextAdapter::make_ref(render_context));
    objects.emplace("pop-mode!", std::make_shared<Function::PopModeBangFunction>());
    objects.emplace("push-mode!", std::make_shared<Function::PushModeBangFunction>());
  }

} // namespace Pixils::Script
