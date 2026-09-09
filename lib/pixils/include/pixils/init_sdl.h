#ifndef PIXILS__INIT_SDL_H
#define PIXILS__INIT_SDL_H

#include <pixils/context.h>

#include <memory>
#include <string>

namespace Pixils
{
  enum class SDLLifetime
  {
    PROCESS,
    HOSTED
  };

  class SDLSession
  {
   public:
    ~SDLSession();
    SDLSession(const SDLSession&) = delete;
    SDLSession& operator=(const SDLSession&) = delete;
    SDLSession(SDLSession&&) = delete;
    SDLSession& operator=(SDLSession&&) = delete;

    RenderContext& render_context();

   private:
    explicit SDLSession(SDLLifetime lifetime);

    RenderContext render_ctx;
    SDLLifetime lifetime;
    bool sdl_initialized = false;
    bool mix_initialized = false;

    friend std::unique_ptr<SDLSession> init_sdl(const std::string&, SDLLifetime);
  };

  std::unique_ptr<SDLSession> init_sdl(const std::string& window_name,
                                       SDLLifetime lifetime = SDLLifetime::PROCESS);
} // namespace Pixils

#endif /* PIXILS__INIT_SDL_H */
