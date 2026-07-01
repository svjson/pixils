#include <pixils/context.h>
#include <pixils/init_sdl.h>

#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_audio.h>
#include <iostream>
#include <optional>

namespace Pixils
{
  std::optional<RenderContext> init_sdl(const std::string& window_name)
  {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
      std::cerr << "Could not initialize media." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

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

    SDL_Window* window = SDL_CreateWindow(window_name.c_str(),
                                          display_w,
                                          display_h,
                                          SDL_WINDOW_FULLSCREEN);
    if (!window)
    {
      std::cerr << "Could not create window." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    SDL_HideCursor();

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == NULL)
    {
      std::cerr << "Could not intialize video renderer." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    if (!MIX_Init())
    {
      std::cerr << "Could not initialize audio mixer." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    SDL_AudioSpec audio_spec{SDL_AUDIO_S16, 2, 44100};
    MIX_Mixer* mixer =
      MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec);
    if (!mixer)
    {
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    return RenderContext{window, renderer, mixer};
  }
} // namespace Pixils
