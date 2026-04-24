
#include "pixils/runtime/render.h"

#include <pixils/hook_context.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/style.h>

namespace Pixils::Runtime
{
  std::vector<Rect> layout_children(const std::vector<std::shared_ptr<View>>& children,
                                    const Rect& parent,
                                    UI::LayoutDirection direction)
  {
    bool row = direction == UI::LayoutDirection::ROW;

    int total_fixed = 0;
    int fill_count = 0;
    for (const auto& child : children)
    {
      UI::Style cs = UI::resolve_style(child->mode->style, child->state);
      if (cs.position && *cs.position == UI::PositionMode::ABSOLUTE) continue;
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
      UI::Style cs = UI::resolve_style(child->mode->style, child->state);
      if (cs.position && *cs.position == UI::PositionMode::ABSOLUTE)
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

  void render_view(Session& session,
                   const std::shared_ptr<View>& view_ptr,
                   const Rect& bounds)
  {
    View& ctx = *view_ptr;
    ctx.bounds = bounds;

    UI::Style style_res = UI::resolve_style(ctx.mode->style, ctx.state);
    if (style_res.hidden && *style_res.hidden) return;

    /**
     * Reset the viewport before any drawing so background coordinates are always
     * in absolute render-target space. Without this, child 0 enters with the
     * parent's content viewport still active, which would offset its background
     * rect by the parent's origin. Children 1..N are fine because each child
     * resets to null at its end - child 0 never got that prior reset.
     */
    SDL_RenderSetViewport(session.render_ctx.renderer, nullptr);

    /** Draw background fill using absolute bounds, now that viewport is null. */
    if (style_res.background && style_res.background->color)
    {
      const SDL_Color& bg = style_res.background->color->to_SDL_Color();
      SDL_SetRenderDrawColor(session.render_ctx.renderer, bg.r, bg.g, bg.b, bg.a);
      SDL_Rect bg_rect = {bounds.x, bounds.y, bounds.w, bounds.h};
      SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_BLEND);
      SDL_RenderFillRect(session.render_ctx.renderer, &bg_rect);
      SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_NONE);
    }

    /** Inset by padding to get the content rect, then set it as the viewport. */
    Rect content = style_res.padding ? style_res.padding->apply_to(bounds) : bounds;

    SDL_Rect viewport = {content.x, content.y, content.w, content.h};
    SDL_RenderSetViewport(session.render_ctx.renderer, &viewport);

    Lisple::obj<HookContext>(*session.hook_args.render_args[1]).current_view = view_ptr;
    Lisple::sptr_rtval_v rargs = {ctx.state, session.hook_args.render_args[1]};
    session.invoke_hook(ctx.mode->render, rargs);

    if (!ctx.children.empty())
    {
      /**
       * Direction comes from the parent's resolved style (default: COLUMN).
       * layout_children computes absolute rects for flow children; absolute-
       * positioned children get zero rects and are placed separately below.
       */
      UI::LayoutDirection direction =
        style_res.direction.value_or(UI::LayoutDirection::COLUMN);
      auto child_rects = layout_children(ctx.children, content, direction);

      for (size_t i = 0; i < ctx.children.size(); i++)
      {
        View& child = *ctx.children[i];
        UI::Style cs = UI::resolve_style(child.mode->style, child.state);
        Rect abs;
        if (cs.position && *cs.position == UI::PositionMode::ABSOLUTE)
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

} // namespace Pixils::Runtime
