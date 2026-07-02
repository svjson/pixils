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
    namespace
    {
      Roo::sptr_val bool_value(bool value)
      {
        return value ? Roo::Constant::BOOL_TRUE : Roo::Constant::BOOL_FALSE;
      }

      RenderContext& render_context(Roo::Context& ctx)
      {
        return Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));
      }

      const Roo::MultiRef LOOP_COUNT({&Roo::Type::NUMBER, &Roo::Type::KEYWORD},
                                     "Number|Keyword");

      int loop_count(Roo::MapSchema::Inspector& opts, int default_value)
      {
        auto value = opts.val("loops");
        if (!value || value->type == Roo::Value::Type::NIL) return default_value;

        if (value->type == Roo::Value::Type::NUMBER) return value->num().get_int();

        if (value->str() == "forever") return -1;

        throw Roo::TypeError("Audio :loops keyword must be :forever");
      }
    } // namespace

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
                                            {"loops", &LOOP_COUNT},
                                            {"volume", &Roo::Type::NUMBER}});

      auto [bundle_id, sound_id] = args[0]->qual();
      auto opts = opts_schema.bind(ctx, *args[1]);

      RenderContext& rc =
        Roo::obj<RenderContext>(*ctx.lookup(ID__PIXILS__RENDER_CONTEXT));

      MIX_Audio* audio = rc.asset_registry->get_sound(bundle_id, sound_id);
      if (!audio) return Roo::number(-1);

      int channel = opts.i32("channel", -1);
      int loops = loop_count(opts, 0);
      float volume = opts.f32("volume", 1.0f);
      int played_channel = rc.play_audio(audio, channel, loops, volume);
      return Roo::number(played_channel);
    }

    FUNC_IMPL(PlayMusicBang,
              MULTI_SIG((FN_ARGS((&Roo::Type::KEYWORD)),
                         EXEC_DISPATCH(&PlayMusicBang::exec_play_music)),
                        (FN_ARGS((&Roo::Type::KEYWORD), (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&PlayMusicBang::exec_play_music_with_opts))));

    EXEC_BODY(PlayMusicBang, exec_play_music)
    {
      Roo::sptr_val_v opt_args = args;
      opt_args.push_back(Roo::map({}));
      return this->exec_play_music_with_opts(ctx, opt_args);
    }

    EXEC_BODY(PlayMusicBang, exec_play_music_with_opts)
    {
      static Roo::MapSchema opts_schema(
        {},
        {{"fade-in-ms", &Roo::Type::NUMBER},
         {"loops", &LOOP_COUNT},
         {"volume", &Roo::Type::NUMBER}});

      auto [bundle_id, music_id] = args[0]->qual();
      auto opts = opts_schema.bind(ctx, *args[1]);

      RenderContext& rc = render_context(ctx);

      MIX_Audio* audio = rc.asset_registry->get_music(bundle_id, music_id);
      if (!audio) return Roo::Constant::BOOL_FALSE;

      return bool_value(rc.play_music(audio,
                                      loop_count(opts, -1),
                                      opts.f32("volume", 1.0f),
                                      opts.i32("fade-in-ms", 0)));
    }

    FUNC_IMPL(StopMusicBang,
              MULTI_SIG((NO_ARGS, EXEC_DISPATCH(&StopMusicBang::exec_stop_music)),
                        (FN_ARGS((&Roo::Type::MAP)),
                         EXEC_DISPATCH(&StopMusicBang::exec_stop_music_with_opts))));

    EXEC_BODY(StopMusicBang, exec_stop_music)
    {
      Roo::sptr_val_v opt_args = args;
      opt_args.push_back(Roo::map({}));
      return this->exec_stop_music_with_opts(ctx, opt_args);
    }

    EXEC_BODY(StopMusicBang, exec_stop_music_with_opts)
    {
      static Roo::MapSchema opts_schema({}, {{"fade-out-ms", &Roo::Type::NUMBER}});
      auto opts = opts_schema.bind(ctx, *args[0]);
      return bool_value(render_context(ctx).stop_music(opts.i32("fade-out-ms", 0)));
    }

    FUNC_IMPL(PauseMusicBang,
              SIG((NO_ARGS, EXEC_DISPATCH(&PauseMusicBang::exec_pause_music))));

    EXEC_BODY(PauseMusicBang, exec_pause_music)
    {
      return bool_value(render_context(ctx).pause_music());
    }

    FUNC_IMPL(ResumeMusicBang,
              SIG((NO_ARGS, EXEC_DISPATCH(&ResumeMusicBang::exec_resume_music))));

    EXEC_BODY(ResumeMusicBang, exec_resume_music)
    {
      return bool_value(render_context(ctx).resume_music());
    }

    FUNC_IMPL(SetMusicVolumeBang,
              SIG((FN_ARGS((&Roo::Type::NUMBER)),
                   EXEC_DISPATCH(&SetMusicVolumeBang::exec_set_music_volume))));

    EXEC_BODY(SetMusicVolumeBang, exec_set_music_volume)
    {
      return bool_value(render_context(ctx).set_music_volume(args[0]->f32()));
    }

    FUNC_IMPL(MusicPlayingP,
              SIG((NO_ARGS, EXEC_DISPATCH(&MusicPlayingP::exec_music_playing))));

    EXEC_BODY(MusicPlayingP, exec_music_playing)
    {
      return bool_value(render_context(ctx).music_playing());
    }
  } // namespace Function

  AudioNamespace::AudioNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__AUDIO))
  {
    values.emplace(FN__MUSIC_PLAYING_P, Function::MusicPlayingP::make());
    values.emplace(FN__PAUSE_MUSIC_BANG, Function::PauseMusicBang::make());
    values.emplace(FN__PLAY_BANG, Function::PlayBang::make());
    values.emplace(FN__PLAY_MUSIC_BANG, Function::PlayMusicBang::make());
    values.emplace(FN__RESUME_MUSIC_BANG, Function::ResumeMusicBang::make());
    values.emplace(FN__SET_MUSIC_VOLUME_BANG, Function::SetMusicVolumeBang::make());
    values.emplace(FN__STOP_MUSIC_BANG, Function::StopMusicBang::make());
  }
} // namespace Pixils::Script
