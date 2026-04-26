
#include "pixils/runtime/event_routing.h"

#include "pixils/runtime/session.h"
#include "pixils/ui/event.h"
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/frame_events.h>
#include <pixils/geom.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/style.h>

#include <lisple/context.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

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
    auto style = UI::resolve_style(view->mode->style, view->state, view->interaction);
    if (style.hidden && *style.hidden) return false;
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

  void EventRouter::update_interaction(View& view, const Point& mouse_pos)
  {
    int mx = mouse_pos.round_x();
    int my = mouse_pos.round_y();

    view.interaction.hovered = view.bounds.w > 0 && mx >= view.bounds.x &&
                               mx < view.bounds.x + view.bounds.w && my >= view.bounds.y &&
                               my < view.bounds.y + view.bounds.h;

    view.interaction.pressed.clear();
    for (auto& [btn, chain] : mouse.button_chains)
    {
      if (!chain.empty())
      {
        if (auto v = chain[0].lock(); v && v.get() == &view)
        {
          view.interaction.pressed.insert(btn);
        }
      }
    }
  }

  static Pixils::Point local_pos(const Pixils::Point& global, const Pixils::Rect& bounds)
  {
    return {global.x - static_cast<float>(bounds.x),
            global.y - static_cast<float>(bounds.y)};
  }

  void EventRouter::handle_mouse_up(FrameEvents& events,
                                    HookArguments& hook_args,
                                    Lisple::Runtime& rt)
  {
    if (mouse.hovered_chain.empty()) return;

    const Point& gp = Lisple::obj<Point>(*events.mouse_pos);

    MouseButtonEvent up_ev;
    up_ev.global_pos = gp;
    up_ev.button = events.mouse_button_up;
    auto up_ev_ref = Script::MouseButtonEventAdapter::make_ref(up_ev);

    MouseButtonEvent click_ev;
    click_ev.global_pos = gp;
    click_ev.button = events.mouse_button_up;
    auto click_ev_ref = Script::MouseButtonEventAdapter::make_ref(click_ev);

    UI::MouseButton up_btn =
      (events.mouse_button_up && events.mouse_button_up->type != Lisple::RTValue::Type::NIL)
        ? UI::mouse_button_from_name(events.mouse_button_up->str())
        : UI::MouseButton::NONE;
    auto pressed_view = mouse.pressed_by(up_btn);

    /**
     * Walk the hovered chain from deepest to root. on_mouse_up and on_click
     * bubble independently: each has its own event instance and propagation_stopped
     * flag, so stopping one does not affect the other. on_click fires on a view
     * if the press originated in its subtree.
     */
    bool pressed_in_subtree = false;
    for (auto& weak_view : mouse.hovered_chain)
    {
      auto view = weak_view.lock();
      if (!view) break;

      if (pressed_view && view.get() == pressed_view.get()) pressed_in_subtree = true;

      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view;

      if (!up_ev.propagation_stopped)
      {
        auto& up_hook = view->mode->on_mouse_up;
        if (up_hook && up_hook->type != Lisple::RTValue::Type::NIL)
        {
          up_ev.local_pos = local_pos(gp, view->bounds);
          Lisple::sptr_rtval_v args = {view->state, up_ev_ref, hook_args.update_args[1]};
          auto new_state = invoke(up_hook, args, rt, view->state);
          if (new_state->type != Lisple::RTValue::Type::NIL) view->state = new_state;
        }
      }

      if (!click_ev.propagation_stopped && pressed_in_subtree)
      {
        auto& click_hook = view->mode->on_click;
        if (click_hook && click_hook->type != Lisple::RTValue::Type::NIL)
        {
          click_ev.local_pos = local_pos(gp, view->bounds);
          Lisple::sptr_rtval_v args = {view->state, click_ev_ref, hook_args.update_args[1]};
          auto new_state = invoke(click_hook, args, rt, view->state);
          if (new_state->type != Lisple::RTValue::Type::NIL) view->state = new_state;
        }
      }

      if (up_ev.propagation_stopped && click_ev.propagation_stopped) break;
    }

    /**
     * Bubble the updated state back up through the hovered chain to root.
     */
    for (size_t i = 0; i + 1 < mouse.hovered_chain.size(); i++)
    {
      auto child = mouse.hovered_chain[i].lock();
      auto parent = mouse.hovered_chain[i + 1].lock();
      if (!child || !parent) break;
      parent->state = merge_state(parent->state, *child, child->state);
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
     * Register this button's hit chain before invoking hooks so that
     * interaction.pressed is current when on_mouse_down reads it.
     * Existing chains for other held buttons are preserved.
     */
    UI::MouseButton btn = (events.mouse_button_down &&
                           events.mouse_button_down->type != Lisple::RTValue::Type::NIL)
                            ? UI::mouse_button_from_name(events.mouse_button_down->str())
                            : UI::MouseButton::NONE;
    auto& chain = mouse.button_chains[btn];
    chain.clear();
    for (auto& view_ptr : hit_chain)
    {
      chain.push_back(std::weak_ptr<View>(view_ptr));
      update_interaction(*view_ptr, gp);
    }

    /**
     * Fire on_mouse_down from deepest hit view upward, stopping when a handler
     * sets propagation_stopped. A single event object is shared across the chain
     * so the stopped flag persists.
     */
    MouseButtonEvent ev;
    ev.global_pos = gp;
    ev.button = events.mouse_button_down;
    auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);

    for (auto& view_ptr : hit_chain)
    {
      auto& hook = view_ptr->mode->on_mouse_down;
      if (!hook || hook->type == Lisple::RTValue::Type::NIL) continue;

      ev.local_pos = local_pos(gp, view_ptr->bounds);
      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view_ptr;
      Lisple::sptr_rtval_v args = {view_ptr->state, ev_ref, hook_args.update_args[1]};
      auto new_state = invoke(hook, args, rt, view_ptr->state);
      if (new_state->type != Lisple::RTValue::Type::NIL) view_ptr->state = new_state;

      if (ev.propagation_stopped) break;
    }

    /**
     * Bubble the updated state back up through ancestors to root.
     */
    for (size_t i = 0; i + 1 < hit_chain.size(); i++)
    {
      auto& child = hit_chain[i];
      auto& parent = hit_chain[i + 1];
      parent->state = merge_state(parent->state, *child, child->state);
    }
  }

  Lisple::sptr_rtval EventRouter::traverse_child(const std::shared_ptr<View>& view_ptr,
                                                 const Lisple::sptr_rtval& parent_state,
                                                 const Point& mouse_pos,
                                                 HookArguments& hook_args,
                                                 Lisple::Runtime& rt)
  {
    View& view = *view_ptr;
    view.state = extract_state(parent_state, view);
    update_interaction(view, mouse_pos);

    Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view_ptr;
    Lisple::sptr_rtval_v uargs = {view.state, hook_args.update_args[1]};
    view.state = invoke(view.mode->update, uargs, rt, view.state);

    std::vector<CustomEvent> emitted_events;
    for (auto& child : view.children)
    {
      view.state = traverse_child(child, view.state, mouse_pos, hook_args, rt);
      child->drain_events(emitted_events);
      emitted_events =
        EventRouter::process_events(view, hook_args.update_args[1], emitted_events, rt);
      for (auto& event : emitted_events)
      {
        view.emitted_events.push_back(event);
      }
      emitted_events.clear();
    }

    return merge_state(parent_state, view, view.state);
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
          Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = old_hovered;
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
            parent->state = merge_state(parent->state, *child, child->state);
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
          Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = new_hovered;
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
            parent->state = merge_state(parent->state, *child, child->state);
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
     * Update root: update interaction flags, call update hook, thread children.
     */
    update_interaction(*root, mouse_pos);

    Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = root;
    Lisple::sptr_rtval_v uargs = {root->state, hook_args.update_args[1]};
    root->state = invoke(root->mode->update, uargs, rt, root->state);

    auto parent_state = root->state;
    for (auto& child : root->children)
      parent_state = traverse_child(child, parent_state, mouse_pos, hook_args, rt);
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
     * Drop chains for any buttons no longer held. Each button's chain is
     * removed independently so releasing one button while another is still
     * held leaves the remaining chains intact.
     */
    if (mouse.has_pressed() &&
        (!events.mouse_button_down ||
         events.mouse_button_down->type == Lisple::RTValue::Type::NIL))
    {
      std::set<UI::MouseButton> held;
      if (events.mouse_held)
      {
        size_t n = Lisple::count(*events.mouse_held);
        for (size_t i = 0; i < n; i++)
          held.insert(
            UI::mouse_button_from_name(Lisple::get_child(*events.mouse_held, i)->str()));
      }
      for (auto it = mouse.button_chains.begin(); it != mouse.button_chains.end();)
      {
        if (!held.count(it->first))
          it = mouse.button_chains.erase(it);
        else
          ++it;
      }
    }
  }

  std::vector<CustomEvent> EventRouter::process_events(View& receiver,
                                                       Lisple::sptr_rtval& view_ctx,
                                                       std::vector<CustomEvent>& events,
                                                       Lisple::Runtime& runtime)
  {
    std::vector<CustomEvent> bubbled_events;
    for (auto& event : events)
    {
      auto it = receiver.mode->event_handlers.find(event.event_key->str());
      if (it == receiver.mode->event_handlers.end() ||
          it->second->type != Lisple::RTValue::Type::FUNCTION)
      {
        bubbled_events.push_back(event);
        continue;
      }
      else
      {
        Lisple::sptr_rtval_v event_args{receiver.state, event.payload, view_ctx};
        receiver.state = EventRouter::invoke(it->second, event_args, runtime);
        if (!event.propagation_stopped)
        {
          bubbled_events.push_back(event);
        }
      }
    }
    return bubbled_events;
  }

} // namespace Pixils::Runtime
