#include <pixils/context.h>
#include <pixils/init_sdl.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_video.h>
#include <iostream>
#include <optional>

namespace Pixils
{
  std::optional<RenderContext> init_sdl(const std::string& window_name)
  {
    std::cout << "Initializing SDL..." << std::endl;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
      std::cerr << "Could not initialize media." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    SDL_DisplayMode display_mode;

    if (SDL_GetCurrentDisplayMode(0, &display_mode) == 0)
    {
      std::cerr << "Warning: Could not read screen resolution from the current display mode"
                << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      display_mode.w = 800;
      display_mode.h = 600;
    }

    SDL_Window* window = SDL_CreateWindow(window_name.c_str(),
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          display_mode.w,
                                          display_mode.h,
                                          SDL_WINDOW_FULLSCREEN_DESKTOP);

    SDL_ShowCursor(SDL_DISABLE);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
      std::cerr << "Could not intialize video renderer." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    int img_flags = IMG_INIT_PNG;

    if ((IMG_Init(img_flags) & img_flags) != img_flags)
    {
      std::cerr << "Could not initialize image loading." << std::endl;
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
      Mix_CloseAudio();
      std::cerr << SDL_GetError() << std::endl;
      return std::nullopt;
    }

    return RenderContext{window, renderer};
  }
} // namespace Pixils
