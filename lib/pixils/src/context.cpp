
#include <pixils/context.h>
#include <pixils/geom.h>

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

namespace Pixils
{

  Dimension RenderContext::get_window_dimension()
  {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    return Dimension{w, h};
  }

  void RenderContext::prepare_frame()
  {
    SDL_GetWindowSize(window, &window_rect.w, &window_rect.h);

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);

    Dimension target_buffer_dim{window_rect.w / this->pixel_size,
                                window_rect.h / this->pixel_size};

    if (this->buffer_texture == nullptr)
    {
      buffer_dim = target_buffer_dim;
      create_and_target_buffer();
    }
    else if (target_buffer_dim != buffer_dim)
    {
      buffer_dim = target_buffer_dim;
      SDL_DestroyTexture(this->buffer_texture);
      create_and_target_buffer();
    }
    else
    {
      clear_buffer();
    }
  }

  void RenderContext::clear_buffer()
  {
    SDL_SetTextureBlendMode(buffer_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(this->renderer, this->buffer_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
  }

  void RenderContext::create_and_target_buffer()
  {
    this->buffer_texture =
        SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                          buffer_dim.w, buffer_dim.h);
    SDL_SetTextureBlendMode(buffer_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(this->renderer, this->buffer_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
  }

  void RenderContext::flush_buffer()
  {
    SDL_SetRenderTarget(this->renderer, nullptr);

    SDL_Rect target{0, 0, buffer_dim.w * pixel_size, buffer_dim.h * pixel_size};

    SDL_RenderCopy(this->renderer, this->buffer_texture, nullptr, &target);
  }

  void RenderContext::finalize_frame()
  {
    SDL_RenderPresent(renderer);
  }
} // namespace Pixils
