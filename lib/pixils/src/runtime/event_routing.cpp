
#include "pixils/runtime/event_routing.h"

#include "pixils/runtime/session.h"
#include "pixils/ui/event.h"
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/frame_events.h>
#include <pixils/geom.h>

#include <lisple/context.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

namespace
{
  Lisple::sptr_rtval extract_child_state(const Lisple::sptr_rtval& parent,
                                         const std::string& id)
  {
    if (!parent || parent->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
    auto val = Lisple::Dict::get_property(parent, Lisple::RTValue::keyword(id));
    return val ? val : Lisple::Constant::NIL;
  }

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

  Pixils::Point local_pos(const Pixils::Point& global, const Pixils::Rect& bounds)
  {
    return {global.x - static_cast<float>(bounds.x),
            global.y - static_cast<float>(bounds.y)};
  }
} // namespace

namespace Pixils::Runtime
{
  Lisple::sptr_rtval EventRouter::invoke(const Lisple::sptr_rtval& fn,
                                         Lisple::sptr_rtval_v& args,
                                         Lisple::Runtime& rt,
                                         const Lisple::sptr_rtval& fallback)
  {
    if (!fn || fn->type == Lisple::RTValue::Type::NIL) return fallback;
    Lisple::Context exec_ctx(rt);
    return fn->exec().execute(exec_ctx, args);
  }

  bool EventRouter::build_hit_chain(std::shared_ptr<View> view,
                                    int mx,
                                    int my,
                                    std::vector<std::shared_ptr<View>>& chain)
  {
    if (view->bounds.w == 0) return false;
    bool hit = mx >= view->bounds.x && mx < view->bounds.x + view->bounds.w &&
               my >= view->bounds.y && my < view->bounds.y + view->bounds.h;
    if (!hit) return false;

    /**
     * Check children in reverse render order: last rendered = visually on top.
     */
    for (auto it = view->children.rbegin(); it != view->children.rend(); ++it)
    {
      if (build_hit_chain(*it, mx, my, chain))
      {
        chain.push_back(view);
        return true;
      }
    }

    chain.push_back(view);
    return true;
  }

  void EventRouter::inject_booleans(View& view, const Point& mouse_pos)
  {
    int mx = mouse_pos.round_x();
    int my = mouse_pos.round_y();

    bool is_hovered = view.bounds.w > 0 && mx >= view.bounds.x &&
                      mx < view.bounds.x + view.bounds.w && my >= view.bounds.y &&
                      my < view.bounds.y + view.bounds.h;

    auto pressed_view = mouse.primary_pressed();
    bool is_pressed = pressed_view && pressed_view.get() == &view;

    if (!view.state || view.state->type == Lisple::RTValue::Type::NIL)
      view.state = Lisple::RTValue::map({});
    else if (view.state->type != Lisple::RTValue::Type::MAP)
      return;

    Lisple::Dict::set_property(
      view.state,
      Lisple::RTValue::keyword("hovered"),
      is_hovered ? Lisple::Constant::BOOL_TRUE : Lisple::Constant::BOOL_FALSE);
    Lisple::Dict::set_property(
      view.state,
      Lisple::RTValue::keyword("pressed"),
      is_pressed ? Lisple::Constant::BOOL_TRUE : Lisple::Constant::BOOL_FALSE);
  }

  void EventRouter::handle_mouse_up(FrameEvents& events,
                                    HookArguments& hook_args,
                                    Lisple::Runtime& rt)
  {
    auto hovered_view = mouse.hovered.lock();
    if (!hovered_view) return;

    const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
    MouseButtonEvent ev;
    ev.global_pos = gp;
    ev.local_pos = local_pos(gp, hovered_view->bounds);
    ev.button = events.mouse_button_up;
    auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);

    auto& up_hook = hovered_view->mode->on_mouse_up;
    if (up_hook && up_hook->type != Lisple::RTValue::Type::NIL)
    {
      Lisple::sptr_rtval_v args = {hovered_view->state, ev_ref, hook_args.update_args[1]};
      auto new_state = invoke(up_hook, args, rt, hovered_view->state);
      if (new_state->type != Lisple::RTValue::Type::NIL) hovered_view->state = new_state;
    }

    /**
     * Fire on_click when the release happens on the view where the press originated.
     */
    auto pressed_view = mouse.primary_pressed();
    if (pressed_view && pressed_view.get() == hovered_view.get())
    {
      auto& click_hook = hovered_view->mode->on_click;
      if (click_hook && click_hook->type != Lisple::RTValue::Type::NIL)
      {
        Lisple::sptr_rtval_v args = {hovered_view->state, ev_ref, hook_args.update_args[1]};
        auto new_state = invoke(click_hook, args, rt, hovered_view->state);
        if (new_state->type != Lisple::RTValue::Type::NIL) hovered_view->state = new_state;
      }
    }

    /**
     * Bubble the updated state back up through the stored hovered chain to root.
     */
    for (size_t i = 0; i + 1 < mouse.hovered_chain.size(); i++)
    {
      auto child = mouse.hovered_chain[i].lock();
      auto parent = mouse.hovered_chain[i + 1].lock();
      if (!child || !parent) break;
      parent->state = merge_child_state(parent->state, child->id, child->state);
    }
  }

  void EventRouter::handle_mouse_down(std::shared_ptr<View> root,
                                      FrameEvents& events,
                                      HookArguments& hook_args,
                                      Lisple::Runtime& rt)
  {
    const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
    int mx = gp.round_x();
    int my = gp.round_y();

    std::vector<std::shared_ptr<View>> hit_chain;
    if (!build_hit_chain(root, mx, my, hit_chain)) return;

    /**
     * Fire on_mouse_down on the deepest hit view that has a handler.
     */
    for (auto& view_ptr : hit_chain)
    {
      auto& hook = view_ptr->mode->on_mouse_down;
      if (!hook || hook->type == Lisple::RTValue::Type::NIL) continue;

      MouseButtonEvent ev;
      ev.global_pos = gp;
      ev.local_pos = local_pos(gp, view_ptr->bounds);
      ev.button = events.mouse_button_down;

      auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);
      Lisple::sptr_rtval_v args = {view_ptr->state, ev_ref, hook_args.update_args[1]};
      auto new_state = invoke(hook, args, rt, view_ptr->state);
      if (new_state->type != Lisple::RTValue::Type::NIL) view_ptr->state = new_state;

      /**
       * Bubble the updated state back up through ancestors to root.
       */
      for (size_t i = 0; i + 1 < hit_chain.size(); i++)
      {
        auto& child = hit_chain[i];
        auto& parent = hit_chain[i + 1];
        parent->state = merge_child_state(parent->state, child->id, child->state);
      }
      break;
    }

    mouse.pressed.clear();
    for (auto& view_ptr : hit_chain)
      mouse.pressed.push_back(std::weak_ptr<View>(view_ptr));
  }

  Lisple::sptr_rtval EventRouter::traverse_child(View& view,
                                                 const Lisple::sptr_rtval& parent_state,
                                                 const Point& mouse_pos,
                                                 HookArguments& hook_args,
                                                 Lisple::Runtime& rt)
  {
    view.state = extract_child_state(parent_state, view.id);
    inject_booleans(view, mouse_pos);

    Lisple::sptr_rtval_v uargs = {view.state, hook_args.update_args[1]};
    view.state = invoke(view.mode->update, uargs, rt, view.state);

    for (auto& child : view.children)
      view.state = traverse_child(*child, view.state, mouse_pos, hook_args, rt);

    return merge_child_state(parent_state, view.id, view.state);
  }

  void EventRouter::traverse(std::shared_ptr<View> root,
                             FrameEvents& events,
                             HookArguments& hook_args,
                             Lisple::Runtime& rt)
  {
    const Point& mouse_pos = Lisple::obj<Point>(*events.mouse_pos);
    int mx = mouse_pos.round_x();
    int my = mouse_pos.round_y();

    /**
     * Determine the new hovered view (deepest hit).
     */
    std::vector<std::shared_ptr<View>> hit_chain;
    build_hit_chain(root, mx, my, hit_chain);
    std::shared_ptr<View> new_hovered = hit_chain.empty() ? nullptr : hit_chain[0];

    /**
     * Fire enter/leave on relevant views if the hovered view has changed.
     */
    auto old_hovered = mouse.hovered.lock();
    if (old_hovered.get() != new_hovered.get())
    {
      if (old_hovered)
      {
        auto& leave_hook = old_hovered->mode->on_mouse_leave;
        if (leave_hook && leave_hook->type != Lisple::RTValue::Type::NIL)
        {
          MouseEvent ev;
          ev.global_pos = mouse_pos;
          ev.local_pos = local_pos(mouse_pos, old_hovered->bounds);
          auto ev_ref = Script::MouseEventAdapter::make_ref(ev);
          Lisple::sptr_rtval_v args = {old_hovered->state, ev_ref, hook_args.update_args[1]};
          auto new_state = invoke(leave_hook, args, rt, old_hovered->state);
          if (new_state->type != Lisple::RTValue::Type::NIL) old_hovered->state = new_state;

          /**
           * hovered_chain still holds the old chain - use it to propagate leave upward.
           */
          for (size_t i = 0; i + 1 < mouse.hovered_chain.size(); i++)
          {
            auto child = mouse.hovered_chain[i].lock();
            auto parent = mouse.hovered_chain[i + 1].lock();
            if (!child || !parent) break;
            parent->state = merge_child_state(parent->state, child->id, child->state);
          }
        }
      }

      mouse.hovered = new_hovered ? std::weak_ptr<View>(new_hovered) : std::weak_ptr<View>{};

      if (new_hovered)
      {
        auto& enter_hook = new_hovered->mode->on_mouse_enter;
        if (enter_hook && enter_hook->type != Lisple::RTValue::Type::NIL)
        {
          MouseEvent ev;
          ev.global_pos = mouse_pos;
          ev.local_pos = local_pos(mouse_pos, new_hovered->bounds);
          auto ev_ref = Script::MouseEventAdapter::make_ref(ev);
          Lisple::sptr_rtval_v args = {new_hovered->state, ev_ref, hook_args.update_args[1]};
          auto new_state = invoke(enter_hook, args, rt, new_hovered->state);
          if (new_state->type != Lisple::RTValue::Type::NIL) new_hovered->state = new_state;

          /**
           * hit_chain holds the new chain - use it to propagate the enter event upward.
           */
          for (size_t i = 0; i + 1 < hit_chain.size(); i++)
          {
            auto& child = hit_chain[i];
            auto& parent = hit_chain[i + 1];
            parent->state = merge_child_state(parent->state, child->id, child->state);
          }
        }
      }
    }

    /**
     * Always refresh hovered_chain so handle_mouse_up can propagate upward.
     */
    mouse.hovered_chain.clear();
    for (auto& v : hit_chain)
      mouse.hovered_chain.push_back(std::weak_ptr<View>(v));

    /**
     * Update root: inject booleans, call update hook, thread children.
     */
    inject_booleans(*root, mouse_pos);

    Lisple::sptr_rtval_v uargs = {root->state, hook_args.update_args[1]};
    root->state = invoke(root->mode->update, uargs, rt, root->state);

    auto parent_state = root->state;
    for (auto& child : root->children)
      parent_state = traverse_child(*child, parent_state, mouse_pos, hook_args, rt);
    root->state = parent_state;
  }

  void EventRouter::update(std::shared_ptr<View> root,
                           FrameEvents& events,
                           HookArguments& hook_args,
                           Lisple::Runtime& rt)
  {
    if (events.mouse_button_up && events.mouse_button_up->type != Lisple::RTValue::Type::NIL)
    {
      handle_mouse_up(events, hook_args, rt);
    }

    traverse(root, events, hook_args, rt);

    if (events.mouse_button_down &&
        events.mouse_button_down->type != Lisple::RTValue::Type::NIL)
    {
      handle_mouse_down(root, events, hook_args, rt);
    }

    /**
     * Clear pressed chain once the button is no longer held.
     */
    if (mouse.has_pressed() &&
        (!events.mouse_button_down ||
         events.mouse_button_down->type == Lisple::RTValue::Type::NIL))
    {
      bool any_held = false;
      if (events.mouse_held)
      {
        size_t n = Lisple::count(*events.mouse_held);
        any_held = n > 0;
      }
      if (!any_held) mouse.pressed.clear();
    }
  }

} // namespace Pixils::Runtime
