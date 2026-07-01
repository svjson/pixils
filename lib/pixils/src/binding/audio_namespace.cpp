#include "pixils/binding/audio_namespace.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>

#include <SDL3_mixer/SDL_mixer.h>
#include <algorithm>
#include <roo/host/schema.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace Function
  {
    FUNC_IMPL(PlayBang,
              MULTI_SIG((FN_ARGS((&Roo::Type::KEYWORD)),
                         EXEC_DISPATCH(&PlayBang::exec_play)),
                        (FN_ARGS((&Roo::Type::KEYWORD), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&PlayBang::exec_play_with_opts))));

    EXEC_BODY(PlayBang, exec_play)
    {
      Roo::sptr_val_v opt_args = args;
      opt_args.push_back(Roo::map({}));
      return this->exec_play_with_opts(ctx, opt_args);
    }

    EXEC_BODY(PlayBang, exec_play_with_opts)
    {
      static Roo::MapSchema opts_schema({},
                                           {{"channel", &Roo::Type::NUMBER},
                                            {"loops", &Roo::Type::NUMBER},
                                            {"volume", &Roo::Type::NUMBER}});

      auto [bundle_id, sound_id] = args[0]->qual();
      auto opts = opts_schema.bind(ctx, *args[1]);

      RenderContext& rc =
        Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      MIX_Audio* audio = rc.asset_registry->get_sound(bundle_id, sound_id);
      if (!audio) return Roo::number(-1);

      int channel = opts.i32("channel", -1);
      int loops = opts.i32("loops", 0);
      float volume = opts.f32("volume", 1.0f);
      int played_channel = rc.play_audio(audio, channel, loops, volume);
      return Roo::number(played_channel);
    }
  } // namespace Function

  AudioNamespace::AudioNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__AUDIO))
  {
    values.emplace(FN__PLAY_BANG, Function::PlayBang::make());
  }
} // namespace Pixils::Script
