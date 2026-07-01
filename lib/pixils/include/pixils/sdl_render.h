#ifndef PIXILS__SDL_RENDER_H
#define PIXILS__SDL_RENDER_H

#include <SDL3/SDL_render.h>

namespace Pixils
{
  inline SDL_FRect to_sdl_frect(const SDL_Rect& rect)
  {
    return SDL_FRect{static_cast<float>(rect.x),
                     static_cast<float>(rect.y),
                     static_cast<float>(rect.w),
                     static_cast<float>(rect.h)};
  }

  inline bool get_texture_size(SDL_Texture* texture, int* w, int* h)
  {
    float fw = 0.0f;
    float fh = 0.0f;
    if (!SDL_GetTextureSize(texture, &fw, &fh)) return false;
    if (w) *w = static_cast<int>(fw);
    if (h) *h = static_cast<int>(fh);
    return true;
  }

  inline bool set_texture_nearest(SDL_Texture* texture)
  {
    return !texture || SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  }

  inline SDL_Texture* create_texture_nearest(SDL_Renderer* renderer,
                                             SDL_PixelFormat format,
                                             SDL_TextureAccess access,
                                             int w,
                                             int h)
  {
    SDL_Texture* texture = SDL_CreateTexture(renderer, format, access, w, h);
    set_texture_nearest(texture);
    return texture;
  }

  inline SDL_Texture* create_texture_from_surface_nearest(SDL_Renderer* renderer,
                                                          SDL_Surface* surface)
  {
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    set_texture_nearest(texture);
    return texture;
  }

  inline bool render_fill_rect(SDL_Renderer* renderer, const SDL_Rect* rect)
  {
    SDL_FRect frect;
    const SDL_FRect* frect_ptr = nullptr;
    if (rect)
    {
      frect = to_sdl_frect(*rect);
      frect_ptr = &frect;
    }
    return SDL_RenderFillRect(renderer, frect_ptr);
  }

  inline bool render_texture(SDL_Renderer* renderer,
                             SDL_Texture* texture,
                             const SDL_Rect* source,
                             const SDL_Rect* dest)
  {
    SDL_FRect source_rect;
    SDL_FRect dest_rect;
    const SDL_FRect* source_ptr = nullptr;
    const SDL_FRect* dest_ptr = nullptr;
    if (source)
    {
      source_rect = to_sdl_frect(*source);
      source_ptr = &source_rect;
    }
    if (dest)
    {
      dest_rect = to_sdl_frect(*dest);
      dest_ptr = &dest_rect;
    }
    return SDL_RenderTexture(renderer, texture, source_ptr, dest_ptr);
  }

  inline bool render_texture(SDL_Renderer* renderer,
                             SDL_Texture* texture,
                             const SDL_Rect* source,
                             const SDL_FRect* dest)
  {
    SDL_FRect source_rect;
    const SDL_FRect* source_ptr = nullptr;
    if (source)
    {
      source_rect = to_sdl_frect(*source);
      source_ptr = &source_rect;
    }
    return SDL_RenderTexture(renderer, texture, source_ptr, dest);
  }

  inline bool render_texture_rotated(SDL_Renderer* renderer,
                                     SDL_Texture* texture,
                                     const SDL_Rect* source,
                                     const SDL_Rect* dest,
                                     double angle,
                                     const SDL_Point* center,
                                     SDL_FlipMode flip)
  {
    SDL_FRect source_rect;
    SDL_FRect dest_rect;
    SDL_FPoint center_point;
    const SDL_FRect* source_ptr = nullptr;
    const SDL_FRect* dest_ptr = nullptr;
    const SDL_FPoint* center_ptr = nullptr;
    if (source)
    {
      source_rect = to_sdl_frect(*source);
      source_ptr = &source_rect;
    }
    if (dest)
    {
      dest_rect = to_sdl_frect(*dest);
      dest_ptr = &dest_rect;
    }
    if (center)
    {
      center_point = SDL_FPoint{static_cast<float>(center->x), static_cast<float>(center->y)};
      center_ptr = &center_point;
    }
    return SDL_RenderTextureRotated(renderer, texture, source_ptr, dest_ptr, angle, center_ptr, flip);
  }
} // namespace Pixils

#endif
