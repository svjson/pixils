#include "pixils/binding/audio_namespace.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>

#include <SDL2/SDL_mixer.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/value.h>

#include <algorithm>

namespace Pixils::Script
{
  namespace
  {
    int normalized_volume_to_mix_level(float volume)
    {
      float clamped = std::clamp(volume, 0.0f, 1.0f);
      return static_cast<int>(clamped * static_cast<float>(MIX_MAX_VOLUME));
    }
  } // namespace

  namespace Function
  {
    FUNC_IMPL(PlayBang,
              MULTI_SIG((FN_ARGS((&Lisple::Type::KEY)), EXEC_DISPATCH(&PlayBang::exec_play)),
                        (FN_ARGS((&Lisple::Type::KEY), (&Lisple::Type::MAP)),
                         EXEC_DISPATCH(&PlayBang::exec_play_with_opts))));

    EXEC_BODY(PlayBang, exec_play)
    {
      Lisple::sptr_rtval_v opt_args = args;
      opt_args.push_back(Lisple::RTValue::map({}));
      return this->exec_play_with_opts(ctx, opt_args);
    }

    EXEC_BODY(PlayBang, exec_play_with_opts)
    {
      static Lisple::MapSchema opts_schema(
        {},
        {{"channel", &Lisple::Type::NUMBER},
         {"loops", &Lisple::Type::NUMBER},
         {"volume", &Lisple::Type::NUMBER}});

      auto [bundle_id, sound_id] = args[0]->qual();
      auto opts = opts_schema.bind(ctx, *args[1]);

      RenderContext& rc =
        Lisple::obj<RenderContext>(*ctx.lookup_value(ID__PIXILS__RENDER_CONTEXT));

      Mix_Chunk* chunk = rc.asset_registry->get_sound(bundle_id, sound_id);
      if (!chunk) return Lisple::RTValue::number(-1);

      int channel = opts.i32("channel", -1);
      int loops = opts.i32("loops", 0);
      float volume = opts.f32("volume", 1.0f);
      int played_channel = Mix_PlayChannel(channel, chunk, loops);

      if (played_channel >= 0)
      {
        Mix_Volume(played_channel, normalized_volume_to_mix_level(volume));
      }

      return Lisple::RTValue::number(played_channel);
    }
  } // namespace Function

  AudioNamespace::AudioNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__AUDIO))
  {
    values.emplace(FN__PLAY_BANG, Function::PlayBang::make());
  }
} // namespace Pixils::Script
