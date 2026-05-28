
#include <pixils/asset/registry.h>
#include <pixils/context.h>
#include <pixils/display.h>
#include <pixils/font_registry.h>
#include <pixils/geom.h>

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <algorithm>

namespace Pixils
{
  RenderContext::RenderContext() = default;

  RenderContext::RenderContext(SDL_Window* window, SDL_Renderer* renderer)
    : window(window)
    , renderer(renderer)
  {
  }

  RenderContext::~RenderContext() = default;
  RenderContext::RenderContext(RenderContext&&) noexcept = default;
  RenderContext& RenderContext::operator=(RenderContext&&) noexcept = default;

  Dimension RenderContext::get_window_dimension()
  {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    return Dimension{w, h};
  }

  Rect RenderContext::application_target_rect(Display& display) const
  {
    SDL_Rect target{0, 0, buffer_dim.w, buffer_dim.h};

    if (target.w <= 0 || target.h <= 0)
    {
      return {0, 0, 0, 0};
    }

    if (display.scaling == Display::Scaling::STRETCH)
    {
      target.w = window_rect.w;
      target.h = window_rect.h;
    }
    else if (display.scaling == Display::Scaling::FIT || display.resolution.pixel_scale > 1)
    {
      int scale = std::min(window_rect.w / target.w, window_rect.h / target.h);
      if (scale < 1) scale = 1;
      target.w *= scale;
      target.h *= scale;
    }

    if (display.align == Display::Alignment::CENTER)
    {
      target.x = window_rect.w / 2 - target.w / 2;
      target.y = window_rect.h / 2 - target.h / 2;
    }

    return {target.x, target.y, target.w, target.h};
  }

  Point RenderContext::window_to_buffer_point(Display& display, int x, int y) const
  {
    Rect target = application_target_rect(display);
    if (target.w <= 0 || target.h <= 0 || buffer_dim.w <= 0 || buffer_dim.h <= 0)
    {
      return {static_cast<float>(x), static_cast<float>(y)};
    }

    return {static_cast<float>(x - target.x) *
              (static_cast<float>(buffer_dim.w) / static_cast<float>(target.w)),
            static_cast<float>(y - target.y) *
              (static_cast<float>(buffer_dim.h) / static_cast<float>(target.h))};
  }

  void RenderContext::begin_frame(Display& display)
  {
    Color& bg = display.background;

    SDL_GetWindowSize(window, &window_rect.w, &window_rect.h);
    set_render_target(nullptr);
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 0xff);
    SDL_RenderClear(renderer);
  }

  void RenderContext::prepare_application_frame(Display& display)
  {
    if (display.resolution.mode == Resolution::Mode::AUTO)
    {
      int ps = display.resolution.pixel_scale;
      display.resolution.dimension = {window_rect.w / ps, window_rect.h / ps};
    }

    Dimension& target_buffer_dim = display.resolution.dimension;

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

    clear_buffer();
  }

  void RenderContext::clear_buffer()
  {
    SDL_SetTextureBlendMode(buffer_texture, SDL_BLENDMODE_BLEND);
    set_render_target(this->buffer_texture);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
  }

  void RenderContext::create_and_target_buffer()
  {
    this->buffer_texture = SDL_CreateTexture(this->renderer,
                                             SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET,
                                             buffer_dim.w,
                                             buffer_dim.h);
    SDL_SetTextureBlendMode(buffer_texture, SDL_BLENDMODE_BLEND);
    set_render_target(this->buffer_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
  }

  void RenderContext::flush_buffer(Display& display)
  {
    set_render_target(nullptr);

    SDL_Rect target = application_target_rect(display).to_SDL_rect();

    SDL_RenderCopy(this->renderer, this->buffer_texture, nullptr, &target);
  }

  void RenderContext::finalize_frame()
  {
    SDL_RenderPresent(renderer);
  }

  void RenderContext::set_render_target(SDL_Texture* target)
  {
    SDL_SetRenderTarget(renderer, target);
    current_render_target = target;
  }

  void RenderContext::set_clip_rect(std::optional<Rect> rect)
  {
    current_clip_rect = rect;
    if (!rect)
    {
      SDL_RenderSetClipRect(renderer, nullptr);
      return;
    }

    SDL_Rect sdl_rect = rect->to_SDL_rect();
    SDL_RenderSetClipRect(renderer, &sdl_rect);
  }
} // namespace Pixils
