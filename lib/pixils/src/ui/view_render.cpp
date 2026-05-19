#include "pixils/ui/view_render.h"

#include <pixils/asset/registry.h>
#include <pixils/benchmark/counters.h>
#include <pixils/context.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/line.h>
#include <pixils/ui/style.h>
#include <pixils/ui/view_layout.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace Pixils::UI
{
  namespace
  {
    std::optional<Rect> intersect_clip(const std::optional<Rect>& clip, const Rect& rect)
    {
      if (!clip) return rect;

      int x1 = std::max(clip->x, rect.x);
      int y1 = std::max(clip->y, rect.y);
      int x2 = std::min(clip->x + clip->w, rect.x + rect.w);
      int y2 = std::min(clip->y + clip->h, rect.y + rect.h);
      if (x2 <= x1 || y2 <= y1) return std::nullopt;
      return Rect{x1, y1, x2 - x1, y2 - y1};
    }

    int scale_factor(const Style& style)
    {
      return std::max(1, style.scale.value_or(1));
    }

    float opacity_factor(const Style& style)
    {
      return std::clamp(style.opacity.value_or(1.0f), 0.0f, 1.0f);
    }

    Uint8 opacity_to_alpha(float opacity)
    {
      return static_cast<Uint8>(std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
    }

    Rect scaled_external_bounds(const Rect& logical_bounds, const Style& style)
    {
      int scale = scale_factor(style);
      return {logical_bounds.x,
              logical_bounds.y,
              logical_bounds.w * scale,
              logical_bounds.h * scale};
    }

    Rect target_rect(const Rect& rect, const Point& origin)
    {
      return {rect.x - origin.round_x(), rect.y - origin.round_y(), rect.w, rect.h};
    }

    int align_offset(int available, int size, Style::Background::Align align)
    {
      switch (align)
      {
      case Style::Background::Align::START:
        return 0;
      case Style::Background::Align::CENTER:
        return static_cast<int>(std::round((available - size) / 2.0f));
      case Style::Background::Align::END:
        return available - size;
      }
      return 0;
    }

    SDL_Rect background_image_dest(const Style::Background& background,
                                   const Rect& bounds,
                                   int source_width,
                                   int source_height,
                                   const Point& origin)
    {
      int width = source_width;
      int height = source_height;

      switch (background.fit.value_or(Style::Background::Fit::NONE))
      {
      case Style::Background::Fit::NONE:
        break;
      case Style::Background::Fit::FILL:
        width = bounds.w;
        height = bounds.h;
        break;
      case Style::Background::Fit::CONTAIN:
      case Style::Background::Fit::COVER:
      {
        if (source_width <= 0 || source_height <= 0 || bounds.w <= 0 || bounds.h <= 0)
        {
          width = 0;
          height = 0;
          break;
        }
        float sx = static_cast<float>(bounds.w) / static_cast<float>(source_width);
        float sy = static_cast<float>(bounds.h) / static_cast<float>(source_height);
        float scale = background.fit == Style::Background::Fit::COVER ? std::max(sx, sy)
                                                                      : std::min(sx, sy);
        width = static_cast<int>(std::round(source_width * scale));
        height = static_cast<int>(std::round(source_height * scale));
        break;
      }
      }

      Style::Background::Align align_x =
        background.align_x.value_or(Style::Background::Align::START);
      Style::Background::Align align_y =
        background.align_y.value_or(Style::Background::Align::START);
      Point offset = background.offset.value_or(Point{0.0f, 0.0f});

      return SDL_Rect{
        bounds.x + align_offset(bounds.w, width, align_x) + offset.round_x() -
          origin.round_x(),
        bounds.y + align_offset(bounds.h, height, align_y) + offset.round_y() -
          origin.round_y(),
        width,
        height,
      };
    }

    void set_clip(Pixils::RenderContext& render_ctx,
                  const std::optional<Rect>& clip,
                  const Point& origin,
                  const std::optional<Rect>& viewport = std::nullopt)
    {
      if (!clip)
      {
        SDL_RenderSetClipRect(render_ctx.renderer, nullptr);
        return;
      }

      int x = viewport ? clip->x - viewport->x : clip->x - origin.round_x();
      int y = viewport ? clip->y - viewport->y : clip->y - origin.round_y();
      SDL_Rect clip_rect = {
        x,
        y,
        clip->w,
        clip->h,
      };
      SDL_RenderSetClipRect(render_ctx.renderer, &clip_rect);
    }

    void render_view_impl(Pixils::RenderContext& render_ctx,
                          Lisple::Runtime& runtime,
                          const Lisple::sptr_val& render_hook_ctx,
                          const std::shared_ptr<Pixils::Runtime::View>& view_ptr,
                          const std::optional<Rect>& inherited_clip,
                          SDL_Texture* target_texture,
                          const Point& origin,
                          bool allow_scale_boundary)
    {
      PIXILS_BENCHMARK_COUNT(render_view_nodes);

      Pixils::Runtime::View& ctx = *view_ptr;
      const Rect bounds = ctx.bounds;

      const Style& style_res = ctx.effective_style;
      if (style_res.visibility &&
          (*style_res.visibility == Style::Visibility::HIDDEN ||
           *style_res.visibility == Style::Visibility::NONE))
        return;

      const float opacity = opacity_factor(style_res);
      if (opacity <= 0.0f) return;

      if (allow_scale_boundary && (scale_factor(style_res) > 1 || opacity < 1.0f))
      {
        if (bounds.w <= 0 || bounds.h <= 0) return;

        PIXILS_BENCHMARK_COUNT(render_offscreen_passes);
        PIXILS_BENCHMARK_COUNT(render_temporary_texture_creations);
        SDL_Texture* texture = SDL_CreateTexture(render_ctx.renderer,
                                                 SDL_PIXELFORMAT_RGBA8888,
                                                 SDL_TEXTUREACCESS_TARGET,
                                                 bounds.w,
                                                 bounds.h);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        SDL_SetRenderTarget(render_ctx.renderer, texture);
        SDL_RenderSetViewport(render_ctx.renderer, nullptr);
        SDL_RenderSetClipRect(render_ctx.renderer, nullptr);
        SDL_SetRenderDrawColor(render_ctx.renderer, 0, 0, 0, 0);
        SDL_RenderClear(render_ctx.renderer);

        render_view_impl(render_ctx,
                         runtime,
                         render_hook_ctx,
                         view_ptr,
                         std::nullopt,
                         texture,
                         {static_cast<float>(bounds.x), static_cast<float>(bounds.y)},
                         false);

        SDL_SetRenderTarget(render_ctx.renderer, target_texture);
        SDL_RenderSetViewport(render_ctx.renderer, nullptr);
        set_clip(render_ctx, inherited_clip, origin);

        Rect external = scaled_external_bounds(bounds, style_res);
        SDL_Rect dest = target_rect(external, origin).to_SDL_rect();
        SDL_SetTextureAlphaMod(texture, opacity_to_alpha(opacity));
        PIXILS_BENCHMARK_COUNT(render_copy_calls);
        SDL_RenderCopy(render_ctx.renderer, texture, nullptr, &dest);

        SDL_DestroyTexture(texture);
        SDL_RenderSetClipRect(render_ctx.renderer, nullptr);
        return;
      }

      auto content_clip = inherited_clip;
      bool content_visible = true;
      if (style_res.clip && *style_res.clip)
      {
        content_clip = intersect_clip(content_clip, style_res.content_rect(bounds));
        content_visible = content_clip.has_value();
      }

      /**
       * Reset the viewport before any drawing so background coordinates are always
       * in absolute render-target space. Without this, child 0 enters with the
       * parent's content viewport still active, which would offset its background
       * rect by the parent's origin. Children 1..N are fine because each child
       * resets to null at its end - child 0 never got that prior reset.
       */
      SDL_RenderSetViewport(render_ctx.renderer, nullptr);
      set_clip(render_ctx, inherited_clip, origin);

      /**
       * Draw background fill using absolute bounds, now that viewport is null.
       */
      if (style_res.background && style_res.background->color)
      {
        auto bg_bounds = intersect_clip(inherited_clip, bounds);
        if (bg_bounds)
        {
          const SDL_Color& bg = style_res.background->color->to_SDL_Color();
          SDL_SetRenderDrawColor(render_ctx.renderer, bg.r, bg.g, bg.b, bg.a);
          SDL_Rect bg_rect = target_rect(*bg_bounds, origin).to_SDL_rect();
          SDL_SetRenderDrawBlendMode(render_ctx.renderer, SDL_BLENDMODE_BLEND);
          PIXILS_BENCHMARK_COUNT(render_fill_rect_calls);
          SDL_RenderFillRect(render_ctx.renderer, &bg_rect);
          SDL_SetRenderDrawBlendMode(render_ctx.renderer, SDL_BLENDMODE_NONE);
        }
      }

      if (style_res.background && style_res.background->image && render_ctx.asset_registry)
      {
        const auto& background = *style_res.background;
        auto [bundle_id, asset_id] = *background.image;
        PIXILS_BENCHMARK_COUNT(render_image_lookup_calls);
        SDL_Texture* texture = render_ctx.asset_registry->get_image(bundle_id, asset_id);
        if (texture)
        {
          SDL_Rect texture_dim = {0, 0, 0, 0};
          PIXILS_BENCHMARK_COUNT(render_texture_queries);
          SDL_QueryTexture(texture, nullptr, nullptr, &texture_dim.w, &texture_dim.h);
          SDL_Rect source = background.source ? background.source->to_SDL_rect()
                                              : SDL_Rect{0, 0, texture_dim.w, texture_dim.h};
          SDL_Rect dest =
            background_image_dest(background, bounds, source.w, source.h, origin);

          set_clip(render_ctx, intersect_clip(inherited_clip, bounds), origin);
          const Uint8 alpha = opacity_to_alpha(background.opacity.value_or(1.0f));
          SDL_SetTextureAlphaMod(texture, alpha);
          PIXILS_BENCHMARK_COUNT(render_copy_calls);
          SDL_RenderCopy(render_ctx.renderer, texture, &source, &dest);
          if (alpha != 255) SDL_SetTextureAlphaMod(texture, 255);
          set_clip(render_ctx, inherited_clip, origin);
        }
      }

      /**
       * Draw border edges in absolute coordinates, on top of the background.
       */
      if (style_res.border)
      {
        Rect target_bounds = target_rect(bounds, origin);
        const Style::BorderStyle& bs = *style_res.border;
        const Style::Trim top_trim = bs.top_trim();
        const Style::Trim right_trim = bs.right_trim();
        const Style::Trim bottom_trim = bs.bottom_trim();
        const Style::Trim left_trim = bs.left_trim();
        const LineSpec top_spec{.thickness = bs.top_thickness(),
                                .color = bs.top_color(),
                                .style = bs.top_line_style(),
                                .trim_start = top_trim.start,
                                .trim_end = top_trim.end};
        const LineSpec right_spec{.thickness = bs.right_thickness(),
                                  .color = bs.right_color(),
                                  .style = bs.right_line_style(),
                                  .trim_start = right_trim.start,
                                  .trim_end = right_trim.end};
        const LineSpec bottom_spec{.thickness = bs.bottom_thickness(),
                                   .color = bs.bottom_color(),
                                   .style = bs.bottom_line_style(),
                                   .trim_start = bottom_trim.start,
                                   .trim_end = bottom_trim.end};
        const LineSpec left_spec{.thickness = bs.left_thickness(),
                                 .color = bs.left_color(),
                                 .style = bs.left_line_style(),
                                 .trim_start = left_trim.start,
                                 .trim_end = left_trim.end};

        render_edge(render_ctx.renderer, target_bounds, Edge::TOP, top_spec);
        render_edge(render_ctx.renderer, target_bounds, Edge::RIGHT, right_spec);
        render_edge(render_ctx.renderer, target_bounds, Edge::BOTTOM, bottom_spec);
        render_edge(render_ctx.renderer, target_bounds, Edge::LEFT, left_spec);

        render_bevel_corner(render_ctx.renderer,
                            target_bounds,
                            Corner::TOP_LEFT,
                            top_spec,
                            left_spec);
        render_bevel_corner(render_ctx.renderer,
                            target_bounds,
                            Corner::BOTTOM_RIGHT,
                            bottom_spec,
                            right_spec);
      }

      Rect content = style_res.content_rect(bounds);

      if (content_visible)
      {
        SDL_Rect viewport = target_rect(content, origin).to_SDL_rect();
        SDL_RenderSetViewport(render_ctx.renderer, &viewport);
        set_clip(render_ctx, content_clip, origin, content);

        PIXILS_BENCHMARK_COUNT(render_hook_calls);
        PIXILS_BENCHMARK_TIME_BLOCK(render_hook_time_ns);
        Lisple::sptr_val_v rargs = {ctx.state, render_hook_ctx};
        Runtime::invoke_hook(runtime, view_ptr, ctx.mode->render, rargs);

        if (!ctx.children.empty())
        {
          for (const auto& child : ctx.children)
          {
            render_view_impl(render_ctx,
                             runtime,
                             render_hook_ctx,
                             child,
                             content_clip,
                             target_texture,
                             origin,
                             true);
          }
        }
      }

      SDL_RenderSetViewport(render_ctx.renderer, nullptr);
      SDL_RenderSetClipRect(render_ctx.renderer, nullptr);
    }
  } // namespace

  void render_view(Pixils::RenderContext& render_ctx,
                   Lisple::Runtime& runtime,
                   const Lisple::sptr_val& render_hook_ctx,
                   const std::shared_ptr<Pixils::Runtime::View>& view_ptr)
  {
    PIXILS_BENCHMARK_COUNT(render_view_calls);
    PIXILS_BENCHMARK_TIME_BLOCK(render_time_ns);

    render_view_impl(render_ctx,
                     runtime,
                     render_hook_ctx,
                     view_ptr,
                     std::nullopt,
                     render_ctx.buffer_texture,
                     POINT__ZERO_ZERO,
                     true);
  }

} // namespace Pixils::UI
