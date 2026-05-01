#include "pixils/ui/view_render.h"

#include <pixils/hook_context.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/style.h>

namespace Pixils::UI
{
  std::vector<Rect> layout_children(
    const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
    const Rect& parent,
    LayoutDirection direction)
  {
    bool row = direction == LayoutDirection::ROW;

    int total_fixed = 0;
    int fill_count = 0;
    for (const auto& child : children)
    {
      Style cs = resolve_style(child->mode->style, child->state, child->interaction);
      if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
      const auto& size_opt = row ? cs.width : cs.height;
      if (size_opt)
        total_fixed += row ? cs.total_width() : cs.total_height();
      else
        fill_count++;
    }

    int available = row ? parent.w : parent.h;
    int fill_size = fill_count > 0 ? (available - total_fixed) / fill_count : 0;

    std::vector<Rect> rects;
    rects.reserve(children.size());

    int pos = row ? parent.x : parent.y;
    for (const auto& child : children)
    {
      Style cs = resolve_style(child->mode->style, child->state, child->interaction);
      if (cs.position && *cs.position == PositionMode::ABSOLUTE)
      {
        rects.push_back({0, 0, 0, 0});
        continue;
      }
      const auto& size_opt = row ? cs.width : cs.height;
      int size = size_opt ? (row ? cs.total_width() : cs.total_height()) : fill_size;
      if (row)
        rects.push_back({pos, parent.y, size, parent.h});
      else
        rects.push_back({parent.x, pos, parent.w, size});
      pos += size;
    }

    return rects;
  }

  void render_view(Pixils::Runtime::Session& session,
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
    SDL_RenderSetViewport(session.render_ctx.renderer, nullptr);

    /**
     * Draw background fill using absolute bounds, now that viewport is null.
     */
    if (style_res.background && style_res.background->color)
    {
      const SDL_Color& bg = style_res.background->color->to_SDL_Color();
      SDL_SetRenderDrawColor(session.render_ctx.renderer, bg.r, bg.g, bg.b, bg.a);
      SDL_Rect bg_rect = {bounds.x, bounds.y, bounds.w, bounds.h};
      SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_BLEND);
      SDL_RenderFillRect(session.render_ctx.renderer, &bg_rect);
      SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_NONE);
    }

    /**
     * Draw border edges in absolute coordinates, on top of the background.
     */
    if (style_res.border)
    {
      const Style::BorderStyle& bs = *style_res.border;

      auto draw_side = [&](int thickness, std::optional<Color> color, SDL_Rect rect)
      {
        if (thickness <= 0 || !color) return;
        const SDL_Color c = color->to_SDL_Color();
        SDL_SetRenderDrawColor(session.render_ctx.renderer, c.r, c.g, c.b, c.a);
        SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(session.render_ctx.renderer, &rect);
        SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_NONE);
      };

      int tt = bs.top_thickness();
      int tr = bs.right_thickness();
      int tb = bs.bottom_thickness();
      int tl = bs.left_thickness();

      draw_side(tt, bs.top_color(), {bounds.x, bounds.y, bounds.w, tt});
      draw_side(tb, bs.bottom_color(), {bounds.x, bounds.y + bounds.h - tb, bounds.w, tb});
      draw_side(tl, bs.left_color(), {bounds.x, bounds.y, tl, bounds.h});
      draw_side(tr, bs.right_color(), {bounds.x + bounds.w - tr, bounds.y, tr, bounds.h});
    }

    Rect content = style_res.content_rect(bounds);

    SDL_Rect viewport = {content.x, content.y, content.w, content.h};
    SDL_RenderSetViewport(session.render_ctx.renderer, &viewport);

    Lisple::sptr_rtval_v rargs = {ctx.state, session.hook_args.render_args[1]};
    Runtime::invoke_hook(session.lisple_runtime, view_ptr, ctx.mode->render, rargs);

    if (!ctx.children.empty())
    {
      /**
       * Direction comes from the parent's resolved style (default: COLUMN).
       * layout_children computes absolute rects for flow children; absolute-
       * positioned children get zero rects and are placed separately below.
       */
      auto direction = style_res.direction.value_or(LayoutDirection::COLUMN);
      auto child_rects = layout_children(ctx.children, content, direction);

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
        render_view(session, ctx.children[i], abs);
      }
    }

    SDL_RenderSetViewport(session.render_ctx.renderer, nullptr);
  }

} // namespace Pixils::UI
