#include <pixils/init_sdl.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <iostream>

namespace Pixils
{
  SDLSession::SDLSession(SDLLifetime lifetime)
    : lifetime(lifetime)
  {
  }

  SDLSession::~SDLSession()
  {
    render_ctx.release_resources();

    if (render_ctx.audio_mixer) MIX_DestroyMixer(render_ctx.audio_mixer);
    render_ctx.audio_mixer = nullptr;

    if (render_ctx.renderer) SDL_DestroyRenderer(render_ctx.renderer);
    render_ctx.renderer = nullptr;

    if (render_ctx.window) SDL_DestroyWindow(render_ctx.window);
    render_ctx.window = nullptr;

    if (mix_initialized) MIX_Quit();
    mix_initialized = false;

    if (sdl_initialized)
    {
      if (lifetime == SDLLifetime::PROCESS)
      {
        SDL_Quit();
      }
      else
      {
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
      }
    }
    sdl_initialized = false;
  }

  RenderContext& SDLSession::render_context()
  {
    return render_ctx;
  }

  std::unique_ptr<SDLSession> init_sdl(const std::string& window_name, SDLLifetime lifetime)
  {
    auto session = std::unique_ptr<SDLSession>(new SDLSession(lifetime));

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
      std::cerr << "Could not initialize media." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      if (lifetime == SDLLifetime::PROCESS) SDL_Quit();
      return nullptr;
    }
    session->sdl_initialized = true;

    int display_w = 800;
    int display_h = 600;

    SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* display_mode = SDL_GetCurrentDisplayMode(display_id);
    if (!display_mode)
    {
      std::cerr << "Warning: Could not read screen resolution from the current display mode"
                << std::endl;
      std::cerr << SDL_GetError() << std::endl;
    }
    else
    {
      display_w = display_mode->w;
      display_h = display_mode->h;
    }

    session->render_ctx.window =
      SDL_CreateWindow(window_name.c_str(), display_w, display_h, SDL_WINDOW_FULLSCREEN);
    if (!session->render_ctx.window)
    {
      std::cerr << "Could not create window." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return nullptr;
    }

    SDL_HideCursor();

    session->render_ctx.renderer = SDL_CreateRenderer(session->render_ctx.window, nullptr);
    if (!session->render_ctx.renderer)
    {
      std::cerr << "Could not intialize video renderer." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return nullptr;
    }

    if (!MIX_Init())
    {
      std::cerr << "Could not initialize audio mixer." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return nullptr;
    }
    session->mix_initialized = true;

    SDL_AudioSpec audio_spec{SDL_AUDIO_S16, 2, 44100};
    session->render_ctx.audio_mixer =
      MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec);
    if (!session->render_ctx.audio_mixer)
    {
      std::cerr << SDL_GetError() << std::endl;
      return nullptr;
    }

    return session;
  }
} // namespace Pixils
