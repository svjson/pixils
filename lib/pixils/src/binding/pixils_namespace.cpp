
#include <pixils/binding/arg_collector.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/frame_events.h>

#include <SDL2/SDL_render.h>
#include <lisple/exec.h>
#include <lisple/host.h>

namespace Pixils
{
  namespace Script
  {
    namespace MapKey
    {
      SHKEY(BUFFER_SIZE, "buffer-size");
      SHKEY(H, "h");
      SHKEY(HELD_KEYS, "held-keys");
      SHKEY(INIT, "init");
      SHKEY(KEY_DOWN, "key-down");
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

        Runtime::Mode mode{.init = proto.get_sptr_property(*MapKey::INIT),
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
      objects.emplace("modes", Lisple::Map::make({}));
      objects.emplace("defmode", std::make_shared<Macro::DefModeMacro>());
      objects.emplace("make-mode", std::make_shared<Function::MakeMode>());
      objects.emplace("render-context", RenderContextAdapter::make_ref(render_context));
    }

  } // namespace Script
} // namespace Pixils
