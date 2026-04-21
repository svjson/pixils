
#include "pixils/runtime/session.h"

#include "pixils/ui/event.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>

#include <lisple/host/object.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace
{
  /**
   * Extract a view's state slice from the parent's state map at the given
   * keyword id. Returns NIL if parent is not a map or the key is absent.
   */
  Lisple::sptr_rtval extract_child_state(const Lisple::sptr_rtval& parent,
                                         const std::string& id)
  {
    if (!parent || parent->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
    auto val = Lisple::Dict::get_property(parent, Lisple::RTValue::keyword(id));
    return val ? val : Lisple::Constant::NIL;
  }

  /**
   * Store child_state into parent's state map under keyword id, returning the
   * updated parent. Creates a new empty map if parent is NIL.
   */
  Lisple::sptr_rtval merge_child_state(Lisple::sptr_rtval parent,
                                       const std::string& id,
                                       Lisple::sptr_rtval child_state)
  {
    auto key = Lisple::RTValue::keyword(id);
    if (!parent || parent->type == Lisple::RTValue::Type::NIL)
      parent = Lisple::RTValue::map({});
    Lisple::Dict::set_property(parent, key, child_state);
    return parent;
  }

} // namespace

namespace Pixils::Runtime
{

  Lisple::sptr_rtval Session::init_view(View& ctx,
                                        const Lisple::sptr_rtval& parent_state)
  {
    if (!this->assets.is_loaded(ctx.mode->name))
      this->assets.load(ctx.mode->name, ctx.mode->resources);

    ctx.state = extract_child_state(parent_state, ctx.id);
    if (ctx.state->type == Lisple::RTValue::Type::NIL) ctx.state = ctx.initial_state;

    Lisple::sptr_rtval_v iargs = {ctx.state, this->hook_args.init_args[1]};
    auto new_state = invoke_hook(ctx.mode->init, iargs);
    if (new_state->type != Lisple::RTValue::Type::NIL) ctx.state = new_state;

    for (auto& grandchild : ctx.children)
      ctx.state = init_view(*grandchild, ctx.state);

    return merge_child_state(parent_state, ctx.id, ctx.state);
  }

  void Session::restore_view_state(View& ctx, const Lisple::sptr_rtval& parent_state)
  {
    ctx.state = extract_child_state(parent_state, ctx.id);
    for (auto& grandchild : ctx.children)
      restore_view_state(*grandchild, ctx.state);
  }

  void Session::update_mode()
  {
    /**
     * Snapshot any mouse button event from this frame into active_mouse_event so
     * update_view can route it through the component tree. The event lives for
     * exactly one call to update_mode and is cleared regardless of propagation outcome.
     */
    if (hook_args.events && hook_args.events->mouse_button_down &&
        hook_args.events->mouse_button_down->type != Lisple::RTValue::Type::NIL)
    {
      MouseEvent ev;
      ev.type = MouseEvent::Type::MOUSE_DOWN;
      ev.position = Lisple::obj<Point>(*hook_args.events->mouse_pos);
      ev.button = hook_args.events->mouse_button_down;
      active_mouse_event = ev;
    }
    else if (hook_args.events && hook_args.events->mouse_button_up &&
             hook_args.events->mouse_button_up->type != Lisple::RTValue::Type::NIL)
    {
      MouseEvent ev;
      ev.type = MouseEvent::Type::MOUSE_UP;
      ev.position = Lisple::obj<Point>(*hook_args.events->mouse_pos);
      ev.button = hook_args.events->mouse_button_up;
      active_mouse_event = ev;
    }
    else
    {
      active_mouse_event.reset();
    }

    auto update_stack = mode_stack.get_update_stack();

    /** Update composition modes below the top, preserving the existing offset semantics. */
    for (size_t i = update_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      View& ctx = *ctx_stack[ctx_idx];

      Lisple::sptr_rtval_v rargs = this->hook_args.update_args;
      rargs[0] = ctx.state;
      ctx.state = invoke_hook(ctx.mode->update, rargs, ctx.state);
      mode_stack.update_state(ctx.state, update_stack.size() - i);
    }

    /** Update the active (top) mode, then thread child updates through the parent state. */
    Lisple::sptr_rtval updated_state = invoke_hook(this->active_mode->mode->update,
                                                   this->hook_args.update_args,
                                                   this->active_mode->state);
    this->active_mode->state = updated_state;

    auto parent_state = this->active_mode->state;
    for (auto& child : this->active_mode->children)
      parent_state = update_view(*child, parent_state);
    this->active_mode->state = parent_state;
    this->hook_args.update_state(parent_state);
  }

  Lisple::sptr_rtval Session::update_view(View& ctx,
                                          const Lisple::sptr_rtval& parent_state)
  {
    ctx.state = extract_child_state(parent_state, ctx.id);

    /**
     * For styled components with known bounds, inject the current hover state
     * into the child's state map before the update hook fires. This allows
     * the component to react to hover in its update hook and lets the style
     * system select the correct variant in render.
     */
    if (hook_args.events && ctx.mode->style && ctx.bounds.w > 0)
    {
      const Point& mp = Lisple::obj<Point>(*hook_args.events->mouse_pos);
      int mx = mp.round_x();
      int my = mp.round_y();
      bool hovered = mx >= ctx.bounds.x && mx < ctx.bounds.x + ctx.bounds.w &&
                     my >= ctx.bounds.y && my < ctx.bounds.y + ctx.bounds.h;

      if (!ctx.state || ctx.state->type == Lisple::RTValue::Type::NIL)
        ctx.state = Lisple::RTValue::map({});
      Lisple::Dict::set_property(
        ctx.state,
        Lisple::RTValue::keyword("hovered"),
        hovered ? Lisple::Constant::BOOL_TRUE : Lisple::Constant::BOOL_FALSE);
    }

    Lisple::sptr_rtval_v uargs = {ctx.state, this->hook_args.update_args[1]};
    ctx.state = invoke_hook(ctx.mode->update, uargs, ctx.state);

    for (auto& grandchild : ctx.children)
      ctx.state = update_view(*grandchild, ctx.state);

    /**
     * Fire mouse event handler if a mouse event occurred this frame, the component
     * has a handler, the cursor is within the component's last-rendered bounds, and
     * propagation has not already been stopped by a deeper component.
     */
    if (active_mouse_event && !active_mouse_event->propagation_stopped && ctx.bounds.w > 0)
    {
      Lisple::sptr_rtval hook = nullptr;

      switch (active_mouse_event->type)
      {
      case MouseEvent::Type::MOUSE_DOWN:
        hook = ctx.mode->on_mouse_down;
        break;
      case MouseEvent::Type::MOUSE_UP:
        hook = ctx.mode->on_mouse_up;
        break;
      }

      if (hook && hook->type != Lisple::RTValue::Type::NIL)
      {
        const Point& mp = active_mouse_event->position;
        int mx = mp.round_x();
        int my = mp.round_y();
        bool hit = mx >= ctx.bounds.x && mx < ctx.bounds.x + ctx.bounds.w &&
                   my >= ctx.bounds.y && my < ctx.bounds.y + ctx.bounds.h;
        if (hit)
        {
          auto ev_ref = Script::MouseEventAdapter::make_ref(*active_mouse_event);
          Lisple::sptr_rtval_v eargs = {ctx.state, ev_ref, this->hook_args.update_args[1]};
          auto new_state = invoke_hook(hook, eargs, ctx.state);
          if (new_state->type != Lisple::RTValue::Type::NIL) ctx.state = new_state;
        }
      }
    }

    return merge_child_state(parent_state, ctx.id, ctx.state);
  }

} // namespace Pixils::Runtime
