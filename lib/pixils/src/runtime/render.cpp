
#include "pixils/runtime/render.h"

#include <pixils/runtime/session.h>

namespace Pixils::Runtime
{
  std::vector<Rect> layout_children(const std::vector<ChildSlot>& slots,
                                    const Rect& parent,
                                    LayoutDirection direction)
  {
    bool row = direction == LayoutDirection::ROW;

    int total_fixed = 0;
    int fill_count = 0;

    for (const auto& slot : slots)
    {
      const auto& constraint = row ? slot.width : slot.height;
      if (constraint.has_value() && constraint->kind == DimensionConstraint::Kind::FIXED)
        total_fixed += constraint->value;
      else
        fill_count++;
    }

    int available = row ? parent.w : parent.h;
    int fill_size = fill_count > 0 ? (available - total_fixed) / fill_count : 0;

    std::vector<Rect> rects;
    int pos = row ? parent.x : parent.y;
    for (const auto& slot : slots)
    {
      const auto& constraint = row ? slot.width : slot.height;
      int size =
        (constraint.has_value() && constraint->kind == DimensionConstraint::Kind::FIXED)
          ? constraint->value
          : fill_size;
      if (row)
        rects.push_back({pos, parent.y, size, parent.h});
      else
        rects.push_back({parent.x, pos, parent.w, size});
      pos += size;
    }

    return rects;
  }

  void render_mode_context(Session& session, ModeContext& ctx, const Rect& bounds)
  {
    ctx.bounds = bounds;

    UI::Style style_res = UI::resolve_style(ctx.mode->style, ctx.state);

    /** Draw background fill at absolute bounds before the viewport is set. */
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

    Lisple::sptr_rtval_v rargs = {ctx.state, session.hook_args.render_args[1]};
    session.invoke_hook(ctx.mode->render, rargs);

    if (!ctx.children.empty())
    {
      /**
       * Layout is computed in local coordinates (origin 0,0 within the viewport),
       * but SDL_RenderSetViewport expects absolute coordinates on the render target.
       * Offset each child rect by the absolute content origin before recursing.
       */
      Rect local_parent = {0, 0, content.w, content.h};
      auto child_rects =
        layout_children(ctx.mode->children, local_parent, ctx.mode->layout_direction);
      for (size_t i = 0; i < ctx.children.size(); i++)
      {
        Rect abs = {content.x + child_rects[i].x,
                    content.y + child_rects[i].y,
                    child_rects[i].w,
                    child_rects[i].h};
        render_mode_context(session, ctx.children[i], abs);
      }
    }

    SDL_RenderSetViewport(session.render_ctx.renderer, nullptr);
  }

} // namespace Pixils::Runtime
