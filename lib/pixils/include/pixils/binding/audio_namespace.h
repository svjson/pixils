#ifndef PIXILS__AUDIO_NAMESPACE_H
#define PIXILS__AUDIO_NAMESPACE_H

#include <roo/exec.h>
#include <roo/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__AUDIO = "pixils.audio";

  inline constexpr std::string_view FN__PLAY_BANG = "play!";
  inline constexpr std::string_view FN__PLAY_MUSIC_BANG = "play-music!";
  inline constexpr std::string_view FN__STOP_MUSIC_BANG = "stop-music!";
  inline constexpr std::string_view FN__PAUSE_MUSIC_BANG = "pause-music!";
  inline constexpr std::string_view FN__RESUME_MUSIC_BANG = "resume-music!";
  inline constexpr std::string_view FN__SET_MUSIC_VOLUME_BANG = "set-music-volume!";
  inline constexpr std::string_view FN__MUSIC_PLAYING_P = "music-playing?";

  namespace Function
  {
    FUNC(PlayBang, play, play_with_opts);
    FUNC(PlayMusicBang, play_music, play_music_with_opts);
    FUNC(StopMusicBang, stop_music, stop_music_with_opts);
    FUNC(PauseMusicBang, pause_music);
    FUNC(ResumeMusicBang, resume_music);
    FUNC(SetMusicVolumeBang, set_music_volume);
    FUNC(MusicPlayingP, music_playing);
  } // namespace Function

  class AudioNamespace : public Roo::Namespace
  {
   public:
    AudioNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__AUDIO_NAMESPACE_H */
