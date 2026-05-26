#include "pixils/ui/line.h"

#include <SDL2/SDL_render.h>
#include <algorithm>
#include <cmath>
#include <optional>

namespace Pixils::UI
{
  namespace
  {
    struct Span
    {
      int x1 = 0;
      int x2 = 0;
    };

    void with_line_color(SDL_Renderer* renderer, const Color& color)
    {
      const SDL_Color c = color.to_SDL_Color();
      SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    }

    void reset_blend_mode(SDL_Renderer* renderer)
    {
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    int clamp_radius(int value, const Rect& bounds)
    {
      return std::clamp(value, 0, std::max(0, std::min(bounds.w, bounds.h) / 2));
    }

    Style::CornerRadius clamp_radius(const Style::CornerRadius& radius, const Rect& bounds)
    {
      return {clamp_radius(radius.tl, bounds),
              clamp_radius(radius.tr, bounds),
              clamp_radius(radius.br, bounds),
              clamp_radius(radius.bl, bounds)};
    }

    int corner_inset(int radius, int corner_y)
    {
      if (radius <= 0) return 0;
      int dy = radius - 1 - corner_y;
      int dx =
        static_cast<int>(std::floor(std::sqrt(std::max(0, radius * radius - dy * dy))));
      return std::max(0, radius - dx);
    }

    std::optional<Span> rounded_span(const Rect& bounds,
                                     const Style::CornerRadius& radius,
                                     int y)
    {
      if (bounds.w <= 0 || bounds.h <= 0) return std::nullopt;
      if (y < bounds.y || y >= bounds.y + bounds.h) return std::nullopt;

      const Style::CornerRadius r = clamp_radius(radius, bounds);
      const int local_y = y - bounds.y;
      int left = bounds.x;
      int right = bounds.x + bounds.w;

      if (local_y < r.tl)
      {
        left = std::max(left, bounds.x + corner_inset(r.tl, local_y));
      }
      if (local_y >= bounds.h - r.bl)
      {
        left = std::max(left, bounds.x + corner_inset(r.bl, bounds.h - 1 - local_y));
      }
      if (local_y < r.tr)
      {
        right = std::min(right, bounds.x + bounds.w - corner_inset(r.tr, local_y));
      }
      if (local_y >= bounds.h - r.br)
      {
        right =
          std::min(right, bounds.x + bounds.w - corner_inset(r.br, bounds.h - 1 - local_y));
      }

      if (right <= left) return std::nullopt;
      return Span{left, right};
    }

    void fill_span(SDL_Renderer* renderer, int x1, int x2, int y)
    {
      if (x2 <= x1) return;
      SDL_Rect rect = {x1, y, x2 - x1, 1};
      SDL_RenderFillRect(renderer, &rect);
    }

    void fill_span(SDL_Renderer* renderer,
                   int x1,
                   int x2,
                   int y,
                   const std::optional<Color>& color)
    {
      if (!color) return;
      with_line_color(renderer, *color);
      fill_span(renderer, x1, x2, y);
      reset_blend_mode(renderer);
    }

    Style::CornerRadius inner_radius(const Style::CornerRadius& outer,
                                     const Style::BorderStyle& border)
    {
      return {
        std::max(0, outer.tl - std::max(border.left_thickness(), border.top_thickness())),
        std::max(0, outer.tr - std::max(border.right_thickness(), border.top_thickness())),
        std::max(0,
                 outer.br - std::max(border.right_thickness(), border.bottom_thickness())),
        std::max(0,
                 outer.bl - std::max(border.left_thickness(), border.bottom_thickness()))};
    }

    void render_solid_edge(SDL_Renderer* renderer,
                           const Rect& bounds,
                           Edge edge,
                           const LineSpec& spec)
    {
      SDL_Rect rect = {bounds.x, bounds.y, 0, 0};
      const int thickness = spec.thickness;

      switch (edge)
      {
      case Edge::TOP:
        rect = {bounds.x + spec.trim_start,
                bounds.y,
                bounds.w - spec.trim_start - spec.trim_end,
                thickness};
        break;
      case Edge::RIGHT:
        rect = {bounds.x + bounds.w - thickness,
                bounds.y + spec.trim_start,
                thickness,
                bounds.h - spec.trim_start - spec.trim_end};
        break;
      case Edge::BOTTOM:
        rect = {bounds.x + spec.trim_end,
                bounds.y + bounds.h - thickness,
                bounds.w - spec.trim_start - spec.trim_end,
                thickness};
        break;
      case Edge::LEFT:
        rect = {bounds.x,
                bounds.y + spec.trim_end,
                thickness,
                bounds.h - spec.trim_start - spec.trim_end};
        break;
      }

      if (rect.w <= 0 || rect.h <= 0) return;
      SDL_RenderFillRect(renderer, &rect);
    }

    void render_bevel_edge(SDL_Renderer* renderer,
                           const Rect& bounds,
                           Edge edge,
                           const LineSpec& spec)
    {
      const int thickness = spec.thickness;
      auto draw_line = [&](int x1, int y1, int x2, int y2)
      {
        if (x2 < x1 || y2 < y1) return;
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
      };

      for (int n = 0; n < thickness; n++)
      {
        switch (edge)
        {
        case Edge::TOP:
          draw_line(bounds.x + spec.trim_start,
                    bounds.y + n,
                    bounds.x + bounds.w - 1 - n - spec.trim_end,
                    bounds.y + n);
          break;
        case Edge::RIGHT:
          draw_line(bounds.x + bounds.w - thickness + n,
                    bounds.y + thickness - n + spec.trim_start,
                    bounds.x + bounds.w - thickness + n,
                    bounds.y + bounds.h - 1 - spec.trim_end);
          break;
        case Edge::BOTTOM:
          draw_line(bounds.x + thickness - n + spec.trim_end,
                    bounds.y + bounds.h - thickness + n,
                    bounds.x + bounds.w - 1 - spec.trim_start,
                    bounds.y + bounds.h - thickness + n);
          break;
        case Edge::LEFT:
          draw_line(bounds.x + n,
                    bounds.y + spec.trim_end,
                    bounds.x + n,
                    bounds.y + bounds.h - 1 - n - spec.trim_start);
          break;
        }
      }
    }

    void render_top_left_bevel_corner(SDL_Renderer* renderer,
                                      const Rect& bounds,
                                      const LineSpec& top,
                                      const LineSpec& left)
    {
      if (top.style != Style::LineStyle::BEVEL || left.style != Style::LineStyle::BEVEL ||
          !top.color || !left.color)
      {
        return;
      }

      const int join = std::min(top.thickness, left.thickness);
      if (join <= 0) return;
      if (top.trim_start > 0 || left.trim_end > 0) return;

      for (int n = 0; n < join; n++)
      {
        with_line_color(renderer, *left.color);
        SDL_RenderDrawLine(renderer, bounds.x, bounds.y + n, bounds.x + n, bounds.y + n);

        if (n < join - 1)
        {
          with_line_color(renderer, *top.color);
          SDL_RenderDrawLine(renderer,
                             bounds.x + n + 1,
                             bounds.y + n,
                             bounds.x + join - 1,
                             bounds.y + n);
        }
      }

      reset_blend_mode(renderer);
    }

    void render_bottom_right_bevel_corner(SDL_Renderer* renderer,
                                          const Rect& bounds,
                                          const LineSpec& bottom,
                                          const LineSpec& right)
    {
      if (bottom.style != Style::LineStyle::BEVEL ||
          right.style != Style::LineStyle::BEVEL || !bottom.color || !right.color)
      {
        return;
      }

      const int join = std::min(bottom.thickness, right.thickness);
      if (join <= 0) return;
      if (bottom.trim_start > 0 || right.trim_end > 0) return;

      for (int n = 0; n < join; n++)
      {
        if (n > 0)
        {
          with_line_color(renderer, *bottom.color);
          SDL_RenderDrawLine(renderer,
                             bounds.x + bounds.w - join,
                             bounds.y + bounds.h - join + n,
                             bounds.x + bounds.w - join + n - 1,
                             bounds.y + bounds.h - join + n);
        }

        with_line_color(renderer, *right.color);
        SDL_RenderDrawLine(renderer,
                           bounds.x + bounds.w - join + n,
                           bounds.y + bounds.h - join + n,
                           bounds.x + bounds.w - 1,
                           bounds.y + bounds.h - join + n);
      }

      reset_blend_mode(renderer);
    }
  } // namespace

  void render_edge(SDL_Renderer* renderer,
                   const Rect& bounds,
                   Edge edge,
                   const LineSpec& spec)
  {
    if (spec.thickness <= 0 || !spec.color) return;

    with_line_color(renderer, *spec.color);

    switch (spec.style)
    {
    case Style::LineStyle::BEVEL:
      render_bevel_edge(renderer, bounds, edge, spec);
      break;
    case Style::LineStyle::SOLID:
    default:
      render_solid_edge(renderer, bounds, edge, spec);
      break;
    }

    reset_blend_mode(renderer);
  }

  void render_bevel_corner(SDL_Renderer* renderer,
                           const Rect& bounds,
                           Corner corner,
                           const LineSpec& horizontal,
                           const LineSpec& vertical)
  {
    switch (corner)
    {
    case Corner::TOP_LEFT:
      render_top_left_bevel_corner(renderer, bounds, horizontal, vertical);
      break;
    case Corner::BOTTOM_RIGHT:
      render_bottom_right_bevel_corner(renderer, bounds, horizontal, vertical);
      break;
    }
  }

  void render_filled_rounded_rect(SDL_Renderer* renderer,
                                  const Rect& bounds,
                                  const Style::CornerRadius& radius,
                                  const Color& color)
  {
    if (bounds.w <= 0 || bounds.h <= 0) return;

    with_line_color(renderer, color);
    for (int y = bounds.y; y < bounds.y + bounds.h; y++)
    {
      auto span = rounded_span(bounds, radius, y);
      if (span) fill_span(renderer, span->x1, span->x2, y);
    }
    reset_blend_mode(renderer);
  }

  void render_rounded_border(SDL_Renderer* renderer,
                             const Rect& bounds,
                             const Style::CornerRadius& radius,
                             const Style::BorderStyle& border)
  {
    if (bounds.w <= 0 || bounds.h <= 0) return;

    const Style::CornerRadius outer_radius = clamp_radius(radius, bounds);
    const Rect inner_bounds = border.apply_to(bounds);
    const bool has_inner = inner_bounds.w > 0 && inner_bounds.h > 0 &&
                           inner_bounds.x < bounds.x + bounds.w &&
                           inner_bounds.y < bounds.y + bounds.h;
    const Style::CornerRadius inner = inner_radius(outer_radius, border);

    for (int y = bounds.y; y < bounds.y + bounds.h; y++)
    {
      auto outer = rounded_span(bounds, outer_radius, y);
      if (!outer) continue;

      std::optional<Span> inner_span = std::nullopt;
      if (has_inner) inner_span = rounded_span(inner_bounds, inner, y);

      if (!inner_span)
      {
        std::optional<Color> color = y < inner_bounds.y ? border.top_color()
                                     : y >= inner_bounds.y + inner_bounds.h
                                       ? border.bottom_color()
                                       : border.left_color();
        if (!color) color = border.right_color();
        fill_span(renderer, outer->x1, outer->x2, y, color);
        continue;
      }

      if (y < inner_bounds.y)
      {
        fill_span(renderer, outer->x1, outer->x2, y, border.top_color());
        continue;
      }
      if (y >= inner_bounds.y + inner_bounds.h)
      {
        fill_span(renderer, outer->x1, outer->x2, y, border.bottom_color());
        continue;
      }

      fill_span(renderer, outer->x1, inner_span->x1, y, border.left_color());
      fill_span(renderer, inner_span->x2, outer->x2, y, border.right_color());
    }
  }
} // namespace Pixils::UI
