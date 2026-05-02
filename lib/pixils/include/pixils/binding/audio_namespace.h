#ifndef PIXILS__AUDIO_NAMESPACE_H
#define PIXILS__AUDIO_NAMESPACE_H

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__AUDIO = "pixils.audio";

  inline constexpr std::string_view FN__PLAY_BANG = "play!";

  namespace Function
  {
    FUNC(PlayBang, play, play_with_opts);
  } // namespace Function

  class AudioNamespace : public Lisple::Namespace
  {
   public:
    AudioNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__AUDIO_NAMESPACE_H */
