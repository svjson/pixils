
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

  void render_child(Session& session, ChildContext& child, const Rect& bounds)
  {
    child.bounds = bounds;

    //    UI::Style style_res = UI::resolve_style(child.mode->style, child.state);
    UI::Style style_res = child.mode->style ? *child.mode->style : UI::Style();
    /** Draw background at absolute bounds before setting the viewport. */
    if (style_res.background)
    {
      if (style_res.background->color)
      {
        const SDL_Color& bg = style_res.background->color->to_SDL_Color();
        SDL_SetRenderDrawColor(session.render_ctx.renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_Rect bg_rect = {bounds.x, bounds.y, bounds.w, bounds.h};
        SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(session.render_ctx.renderer, &bg_rect);
        SDL_SetRenderDrawBlendMode(session.render_ctx.renderer, SDL_BLENDMODE_NONE);
      }
    }

    /** Inset bounds by padding to get the content area. */
    Rect content = style_res.padding ? style_res.padding->apply_to(bounds) : bounds;

    SDL_Rect viewport = {content.x, content.y, content.w, content.h};
    SDL_RenderSetViewport(session.render_ctx.renderer, &viewport);

    // if (style_res.color)
    // {
    //   const SDL_Color& fg = *style_res.color;
    //   SDL_SetRenderDrawColor(render_ctx.renderer, fg.r, fg.g, fg.b, fg.a);
    // }

    Lisple::sptr_rtval_v rargs = {child.state, session.hook_args.render_args[1]};
    session.invoke_hook(child.mode->render, rargs);

    if (!child.children.empty())
    {
      /**
       * Child bounds here are local (origin at 0,0 within the viewport), but
       * SDL_RenderSetViewport expects absolute coordinates on the render target.
       * Offset by the parent's absolute content position when recursing.
       */
      Rect local_parent = {0, 0, content.w, content.h};
      auto grandchild_rects =
        layout_children(child.mode->children, local_parent, child.mode->layout_direction);
      for (size_t i = 0; i < child.children.size(); i++)
      {
        Rect abs = {content.x + grandchild_rects[i].x,
                    content.y + grandchild_rects[i].y,
                    grandchild_rects[i].w,
                    grandchild_rects[i].h};
        render_child(session, child.children[i], abs);
      }
    }

    SDL_RenderSetViewport(session.render_ctx.renderer, nullptr);
  }

} // namespace Pixils::Runtime
