#include <pixils/context.h>

#include <SDL2/SDL.h>
#include <iostream>
#include <optional>

namespace Pixils
{
  std::optional<RenderContext> init_sdl(const std::string& window_name);
}
