
#include "pixils/runtime/event_routing.h"

#include "pixils/runtime/session.h"
#include "pixils/ui/event.h"
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/frame_events.h>
#include <pixils/geom.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/style.h>

#include <algorithm>
#include <functional>
#include <lisple/context.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    Lisple::sptr_rtval invoke(const Lisple::sptr_rtval& fn,
                              Lisple::sptr_rtval_v& args,
                              Lisple::Runtime& rt,
                              const Lisple::sptr_rtval& fallback = Lisple::Constant::NIL)
    {
      if (!fn || fn->type == Lisple::RTValue::Type::NIL) return fallback;
      Lisple::Context exec_ctx(rt);
      return fn->exec().execute(exec_ctx, args);
    }

    Point local_pos(const Point& global, const Rect& bounds)
    {
      return {global.x - static_cast<float>(bounds.x),
              global.y - static_cast<float>(bounds.y)};
    }

    /** Locks a weak_ptr chain to shared_ptrs, stopping at the first expired entry. */
    std::vector<std::shared_ptr<View>> lock_chain(
      const std::vector<std::weak_ptr<View>>& wchain)
    {
      std::vector<std::shared_ptr<View>> result;
      result.reserve(wchain.size());
      for (auto& w : wchain)
      {
        if (auto s = w.lock())
        {
          result.push_back(s);
        }
        else
        {
          break;
        }
      }
      return result;
    }

    void restore_subtree_state(const std::shared_ptr<View>& view,
                               const Lisple::sptr_rtval& parent_state)
    {
      view->state = extract_state(parent_state, *view);
      for (auto& child : view->children)
      {
        restore_subtree_state(child, view->state);
      }
    }

    /**
     * Fire a single hook on view, updating view->state if the hook returns non-NIL.
     * When a non-leaf hook updates a bound state branch, immediately resync the
     * descendants so later hook phases do not merge stale child state back over it.
     */
    void fire_hook_on_view(const std::shared_ptr<View>& view,
                           const Lisple::sptr_rtval& hook,
                           const Lisple::sptr_rtval& ev_ref,
                           HookArguments& hook_args,
                           Lisple::Runtime& rt)
    {
      if (!hook || hook->type == Lisple::RTValue::Type::NIL) return;
      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view;
      Lisple::sptr_rtval_v args = {view->state, ev_ref, hook_args.update_args[1]};
      auto new_state = invoke(hook, args, rt, view->state);
      if (new_state->type != Lisple::RTValue::Type::NIL)
      {
        view->state = new_state;
        for (auto& child : view->children)
        {
          restore_subtree_state(child, view->state);
        }
      }
    }

    void run_update_hook(const std::shared_ptr<View>& view,
                         HookArguments& hook_args,
                         Lisple::Runtime& rt)
    {
      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view;
      Lisple::sptr_rtval_v uargs = {view->state, hook_args.update_args[1]};
      view->state = invoke(view->mode->update, uargs, rt, view->state);
    }

    void propagate_state_up_chain(const std::vector<std::shared_ptr<View>>& chain)
    {
      for (size_t i = 0; i + 1 < chain.size(); i++)
      {
        chain[i + 1]->state =
          merge_state(chain[i + 1]->state, *chain[i], chain[i]->state);
      }
    }

    /**
     * Bubble a hook through chain from deepest to root. For each view: if propagation
     * has not been stopped, calls set_local_pos then fires the named hook field.
     * Immediately merges each view's updated state into its parent before the next
     * view's hook runs. State propagation continues to root even when propagation
     * is stopped, so no separate bubble-up pass is ever needed after calling this.
     */
    void bubble_hook(const std::vector<std::shared_ptr<View>>& chain,
                     Lisple::sptr_rtval Mode::* hook_field,
                     const Lisple::sptr_rtval& ev_ref,
                     bool& propagation_stopped,
                     const std::function<void(const Rect&)>& set_local_pos,
                     HookArguments& hook_args,
                     Lisple::Runtime& rt)
    {
      for (size_t i = 0; i < chain.size(); i++)
      {
        auto& view = chain[i];
        if (!propagation_stopped)
        {
          set_local_pos(view->bounds);
          fire_hook_on_view(view, view->mode->*hook_field, ev_ref, hook_args, rt);
        }
        if (i + 1 < chain.size())
        {
          chain[i + 1]->state = merge_state(chain[i + 1]->state, *view, view->state);
        }
      }
    }

    /**
     * Depth-first hit test returning the deepest view whose bounds contain
     * (mx, my), together with the ancestor chain [deepest, ..., root].
     * Returns false if root itself is not hit.
     */
    bool build_hit_chain(std::shared_ptr<View> view,
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

    void bubble_child_events_to_subject(View& subject,
                                        Lisple::sptr_rtval* subject_parent_state,
                                        const std::shared_ptr<View>& child,
                                        Lisple::sptr_rtval& view_ctx,
                                        Lisple::Runtime& rt)
    {
      std::vector<CustomEvent> emitted_events;
      child->drain_events(emitted_events);
      emitted_events =
        EventRouter::process_events(subject, subject_parent_state, view_ctx, emitted_events, rt);

      for (auto& event : emitted_events)
      {
        subject.emitted_events.push_back(event);
      }
    }

  } // namespace

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
      for (auto& weak_v : chain)
      {
        if (auto v = weak_v.lock(); v && v.get() == &view)
        {
          view.interaction.pressed.insert(btn);
          break;
        }
      }
    }
  }

  void EventRouter::handle_mouse_up(FrameEvents& events,
                                    HookArguments& hook_args,
                                    Lisple::Runtime& rt)
  {
    if (mouse.hovered_chain.empty()) return;

    const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
    auto chain = lock_chain(mouse.hovered_chain);
    if (chain.empty()) return;

    UI::MouseButton up_btn =
      (events.mouse_button_up && events.mouse_button_up->type != Lisple::RTValue::Type::NIL)
        ? UI::mouse_button_from_name(events.mouse_button_up->str())
        : UI::MouseButton::NONE;

    {
      MouseButtonEvent ev;
      ev.global_pos = gp;
      ev.button = events.mouse_button_up;
      auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);
      bubble_hook(
        chain,
        &Mode::on_mouse_up,
        ev_ref,
        ev.propagation_stopped,
        [&](const Rect& b) { ev.local_pos = local_pos(gp, b); },
        hook_args,
        rt);
    }

    /**
     * on_click fires only on the pressed view and its ancestors in the hovered chain.
     * Trimming the chain to start at pressed_view eliminates the need for a
     * pressed_in_subtree flag: if pressed_view is not in the chain (cursor left the
     * view before release), find_if returns end and no click fires.
     */
    auto pressed_view = mouse.pressed_by(up_btn);
    if (pressed_view)
    {
      auto it = std::find_if(chain.begin(),
                             chain.end(),
                             [&](const auto& v) { return v.get() == pressed_view.get(); });
      if (it != chain.end())
      {
        MouseButtonEvent click_ev;
        click_ev.global_pos = gp;
        click_ev.button = events.mouse_button_up;
        auto click_ev_ref = Script::MouseButtonEventAdapter::make_ref(click_ev);
        std::vector<std::shared_ptr<View>> click_chain(it, chain.end());
        bubble_hook(
          click_chain,
          &Mode::on_click,
          click_ev_ref,
          click_ev.propagation_stopped,
          [&](const Rect& b) { click_ev.local_pos = local_pos(gp, b); },
          hook_args,
          rt);
      }
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
    auto& btn_chain = mouse.button_chains[btn];
    btn_chain.clear();
    for (auto& view_ptr : hit_chain)
    {
      btn_chain.push_back(std::weak_ptr<View>(view_ptr));
      update_interaction(*view_ptr, gp);
    }

    MouseButtonEvent ev;
    ev.global_pos = gp;
    ev.button = events.mouse_button_down;
    auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);
    bubble_hook(
      hit_chain,
      &Mode::on_mouse_down,
      ev_ref,
      ev.propagation_stopped,
      [&](const Rect& b) { ev.local_pos = local_pos(gp, b); },
      hook_args,
      rt);
  }

  void EventRouter::handle_mouse_motion(FrameEvents& events,
                                        HookArguments& hook_args,
                                        Lisple::Runtime& rt)
  {
    auto chain = lock_chain(mouse.hovered_chain);
    if (chain.empty()) return;

    const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
    MouseEvent ev;
    ev.global_pos = gp;
    auto ev_ref = Script::MouseEventAdapter::make_ref(ev);
    bubble_hook(
      chain,
      &Mode::on_mouse_motion,
      ev_ref,
      ev.propagation_stopped,
      [&](const Rect& b) { ev.local_pos = local_pos(gp, b); },
      hook_args,
      rt);
  }

  void EventRouter::traverse_child(const std::shared_ptr<View>& view_ptr,
                                   Lisple::sptr_rtval* parent_state,
                                   const Point& mouse_pos,
                                   HookArguments& hook_args,
                                   Lisple::Runtime& rt)
  {
    View& view = *view_ptr;

    if (parent_state)
    {
      view.state = extract_state(*parent_state, view);
    }

    update_interaction(view, mouse_pos);
    run_update_hook(view_ptr, hook_args, rt);

    for (auto& child : view.children)
    {
      traverse_child(child, &view.state, mouse_pos, hook_args, rt);
      bubble_child_events_to_subject(
        view, parent_state, child, hook_args.update_args[1], rt);
    }

    if (parent_state)
    {
      *parent_state = merge_state(*parent_state, view, view.state);
    }
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
     * Fire enter/leave on the specific view that changed hover status. These do not
     * bubble - they fire only on the one view whose status changed. State is then
     * propagated through the respective chain so ancestors reflect the change.
     */
    auto old_hovered = mouse.hovered.lock();
    if (old_hovered.get() != new_hovered.get())
    {
      if (old_hovered)
      {
        MouseEvent leave_ev;
        leave_ev.global_pos = mouse_pos;
        leave_ev.local_pos = local_pos(mouse_pos, old_hovered->bounds);
        auto ev_ref = Script::MouseEventAdapter::make_ref(leave_ev);
        fire_hook_on_view(old_hovered,
                          old_hovered->mode->on_mouse_leave,
                          ev_ref,
                          hook_args,
                          rt);

        auto old_chain = lock_chain(mouse.hovered_chain);
        propagate_state_up_chain(old_chain);
      }

      mouse.hovered = new_hovered ? std::weak_ptr<View>(new_hovered) : std::weak_ptr<View>{};

      if (new_hovered)
      {
        MouseEvent enter_ev;
        enter_ev.global_pos = mouse_pos;
        enter_ev.local_pos = local_pos(mouse_pos, new_hovered->bounds);
        auto ev_ref = Script::MouseEventAdapter::make_ref(enter_ev);
        fire_hook_on_view(new_hovered,
                          new_hovered->mode->on_mouse_enter,
                          ev_ref,
                          hook_args,
                          rt);

        propagate_state_up_chain(hit_chain);
      }
    }

    /**
     * Always refresh hovered_chain so handle_mouse_up and handle_mouse_motion
     * have access to the current chain.
     */
    mouse.hovered_chain.clear();
    for (auto& v : hit_chain)
    {
      mouse.hovered_chain.push_back(std::weak_ptr<View>(v));
    }

    traverse_child(root, nullptr, mouse_pos, hook_args, rt);
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

    if (events.mouse_button_down &&
        events.mouse_button_down->type != Lisple::RTValue::Type::NIL)
    {
      handle_mouse_down(root, events, hook_args, rt);
    }

    traverse(root, events, hook_args, rt);

    if (events.mouse_moved)
    {
      handle_mouse_motion(events, hook_args, rt);
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
        {
          held.insert(
            UI::mouse_button_from_name(Lisple::get_child(*events.mouse_held, i)->str()));
        }
      }
      for (auto it = mouse.button_chains.begin(); it != mouse.button_chains.end();)
      {
        if (!held.count(it->first))
        {
          it = mouse.button_chains.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }

  std::vector<CustomEvent> EventRouter::process_events(View& receiver,
                                                       Lisple::sptr_rtval* parent_state,
                                                       Lisple::sptr_rtval& view_ctx,
                                                       std::vector<CustomEvent>& events,
                                                       Lisple::Runtime& runtime,
                                                       bool* receiver_state_updated)
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
        auto new_state = invoke(it->second, event_args, runtime, receiver.state);
        if (new_state->type != Lisple::RTValue::Type::NIL)
        {
          receiver.state = new_state;
          if (receiver_state_updated) *receiver_state_updated = true;
          for (auto& child : receiver.children)
          {
            restore_subtree_state(child, receiver.state);
          }
          if (parent_state)
          {
            *parent_state = merge_state(*parent_state, receiver, receiver.state);
          }
        }
        if (!event.propagation_stopped)
        {
          bubbled_events.push_back(event);
        }
      }
    }
    return bubbled_events;
  }

} // namespace Pixils::Runtime
