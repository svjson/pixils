#include "pixils/ui/view_render.h"

#include <pixils/context.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/line.h>
#include <pixils/ui/style.h>
#include <pixils/ui/view_layout.h>

namespace Pixils::UI
{
  void render_view(Pixils::RenderContext& render_ctx,
                   Lisple::Runtime& runtime,
                   const Lisple::sptr_rtval& render_hook_ctx,
                   const std::shared_ptr<Pixils::Runtime::View>& view_ptr,
                   const Rect& bounds)
  {
    Pixils::Runtime::View& ctx = *view_ptr;
    ctx.bounds = bounds;

    Style style_res = resolve_style(ctx.mode->style, ctx.state, ctx.interaction);
    if (style_res.hidden && *style_res.hidden) return;

    /**
     * Reset the viewport before any drawing so background coordinates are always
     * in absolute render-target space. Without this, child 0 enters with the
     * parent's content viewport still active, which would offset its background
     * rect by the parent's origin. Children 1..N are fine because each child
     * resets to null at its end - child 0 never got that prior reset.
     */
    SDL_RenderSetViewport(render_ctx.renderer, nullptr);

    /**
     * Draw background fill using absolute bounds, now that viewport is null.
     */
    if (style_res.background && style_res.background->color)
    {
      const SDL_Color& bg = style_res.background->color->to_SDL_Color();
      SDL_SetRenderDrawColor(render_ctx.renderer, bg.r, bg.g, bg.b, bg.a);
      SDL_Rect bg_rect = {bounds.x, bounds.y, bounds.w, bounds.h};
      SDL_SetRenderDrawBlendMode(render_ctx.renderer, SDL_BLENDMODE_BLEND);
      SDL_RenderFillRect(render_ctx.renderer, &bg_rect);
      SDL_SetRenderDrawBlendMode(render_ctx.renderer, SDL_BLENDMODE_NONE);
    }

    /**
     * Draw border edges in absolute coordinates, on top of the background.
     */
    if (style_res.border)
    {
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

      render_edge(render_ctx.renderer, bounds, Edge::TOP, top_spec);
      render_edge(render_ctx.renderer, bounds, Edge::RIGHT, right_spec);
      render_edge(render_ctx.renderer, bounds, Edge::BOTTOM, bottom_spec);
      render_edge(render_ctx.renderer, bounds, Edge::LEFT, left_spec);

      render_bevel_corner(render_ctx.renderer,
                          bounds,
                          Corner::TOP_LEFT,
                          top_spec,
                          left_spec);
      render_bevel_corner(render_ctx.renderer,
                          bounds,
                          Corner::BOTTOM_RIGHT,
                          bottom_spec,
                          right_spec);
    }

    Rect content = style_res.content_rect(bounds);

    SDL_Rect viewport = {content.x, content.y, content.w, content.h};
    SDL_RenderSetViewport(render_ctx.renderer, &viewport);

    Lisple::sptr_rtval_v rargs = {ctx.state, render_hook_ctx};
    Runtime::invoke_hook(runtime, view_ptr, ctx.mode->render, rargs);

    if (!ctx.children.empty())
    {
      /**
       * Direction comes from the parent's resolved style (default: COLUMN).
       * layout_children computes absolute rects for flow children; absolute-
       * positioned children get zero rects and are placed separately below.
       */
      auto direction = style_res.direction.value_or(LayoutDirection::COLUMN);
      auto child_rects =
        layout_children(ctx.children, content, runtime, render_hook_ctx, direction);

      for (size_t i = 0; i < ctx.children.size(); i++)
      {
        Pixils::Runtime::View& child = *ctx.children[i];
        Style cs = resolve_style(child.mode->style, child.state, child.interaction);
        Rect abs;
        if (cs.position && *cs.position == PositionMode::ABSOLUTE)
        {
          int top = cs.top.value_or(0);
          int left = cs.left.value_or(0);
          int w = cs.width.has_value() ? cs.total_width() : content.w;
          int h = cs.height.has_value() ? cs.total_height() : content.h;
          abs = {content.x + left, content.y + top, w, h};
        }
        else
        {
          abs = child_rects[i];
        }
        render_view(render_ctx, runtime, render_hook_ctx, ctx.children[i], abs);
      }
    }

    SDL_RenderSetViewport(render_ctx.renderer, nullptr);
  }

} // namespace Pixils::UI
