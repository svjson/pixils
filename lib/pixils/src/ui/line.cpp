#include "pixils/ui/line.h"

#include <SDL2/SDL_render.h>

namespace Pixils::UI
{
  namespace
  {
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

    void render_solid_edge(SDL_Renderer* renderer,
                           const Rect& bounds,
                           Edge edge,
                           int thickness)
    {
      SDL_Rect rect = {bounds.x, bounds.y, 0, 0};

      switch (edge)
      {
      case Edge::TOP:
        rect = {bounds.x, bounds.y, bounds.w, thickness};
        break;
      case Edge::RIGHT:
        rect = {bounds.x + bounds.w - thickness, bounds.y, thickness, bounds.h};
        break;
      case Edge::BOTTOM:
        rect = {bounds.x, bounds.y + bounds.h - thickness, bounds.w, thickness};
        break;
      case Edge::LEFT:
        rect = {bounds.x, bounds.y, thickness, bounds.h};
        break;
      }

      SDL_RenderFillRect(renderer, &rect);
    }

    void render_bevel_edge(SDL_Renderer* renderer,
                           const Rect& bounds,
                           Edge edge,
                           int thickness)
    {
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
          draw_line(bounds.x, bounds.y + n, bounds.x + bounds.w - 1 - n, bounds.y + n);
          break;
        case Edge::RIGHT:
          draw_line(bounds.x + bounds.w - thickness + n,
                    bounds.y + thickness - n,
                    bounds.x + bounds.w - thickness + n,
                    bounds.y + bounds.h - 1);
          break;
        case Edge::BOTTOM:
          draw_line(bounds.x + thickness - n,
                    bounds.y + bounds.h - thickness + n,
                    bounds.x + bounds.w - 1,
                    bounds.y + bounds.h - thickness + n);
          break;
        case Edge::LEFT:
          draw_line(bounds.x + n, bounds.y, bounds.x + n, bounds.y + bounds.h - 1 - n);
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
      render_bevel_edge(renderer, bounds, edge, spec.thickness);
      break;
    case Style::LineStyle::SOLID:
    default:
      render_solid_edge(renderer, bounds, edge, spec.thickness);
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
} // namespace Pixils::UI
