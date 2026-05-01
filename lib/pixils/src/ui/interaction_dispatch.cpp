#include "pixils/ui/interaction_dispatch.h"

#include "pixils/runtime/event_routing.h"
#include "pixils/runtime/session.h"
#include "pixils/ui/event.h"
#include "pixils/ui/view_lifecycle.h"
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
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

namespace Pixils::UI
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

    std::vector<std::shared_ptr<Runtime::View>> lock_chain(
      const std::vector<std::weak_ptr<Runtime::View>>& wchain)
    {
      std::vector<std::shared_ptr<Runtime::View>> result;
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

    void fire_hook_on_view(const std::shared_ptr<Runtime::View>& view,
                           const Lisple::sptr_rtval& hook,
                           const Lisple::sptr_rtval& ev_ref,
                           Runtime::HookArguments& hook_args,
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
          restore_view_tree(child, view->state);
        }
      }
    }

    void run_update_hook(const std::shared_ptr<Runtime::View>& view,
                         Runtime::HookArguments& hook_args,
                         Lisple::Runtime& rt)
    {
      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view;
      Lisple::sptr_rtval_v uargs = {view->state, hook_args.update_args[1]};
      view->state = invoke(view->mode->update, uargs, rt, view->state);
    }

    void propagate_state_up_chain(const std::vector<std::shared_ptr<Runtime::View>>& chain)
    {
      for (size_t i = 0; i + 1 < chain.size(); i++)
      {
        chain[i + 1]->state =
          Runtime::merge_state(chain[i + 1]->state, *chain[i], chain[i]->state);
      }
    }

    void bubble_hook(const std::vector<std::shared_ptr<Runtime::View>>& chain,
                     Lisple::sptr_rtval Runtime::Mode::* hook_field,
                     const Lisple::sptr_rtval& ev_ref,
                     bool& propagation_stopped,
                     const std::function<void(const Rect&)>& set_local_pos,
                     Runtime::HookArguments& hook_args,
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
          chain[i + 1]->state =
            Runtime::merge_state(chain[i + 1]->state, *view, view->state);
        }
      }
    }

    bool build_hit_chain(std::shared_ptr<Runtime::View> view,
                         int mx,
                         int my,
                         std::vector<std::shared_ptr<Runtime::View>>& chain)
    {
      if (view->bounds.w == 0) return false;
      auto style = resolve_style(view->mode->style, view->state, view->interaction);
      if (style.hidden && *style.hidden) return false;
      bool hit = mx >= view->bounds.x && mx < view->bounds.x + view->bounds.w &&
                 my >= view->bounds.y && my < view->bounds.y + view->bounds.h;
      if (!hit) return false;

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

    void bubble_child_events_to_subject(Runtime::View& subject,
                                        Lisple::sptr_rtval* subject_parent_state,
                                        const std::shared_ptr<Runtime::View>& child,
                                        Lisple::sptr_rtval& view_ctx,
                                        Lisple::Runtime& rt)
    {
      std::vector<CustomEvent> emitted_events;
      child->drain_events(emitted_events);
      emitted_events = Runtime::process_view_events(subject,
                                                    subject_parent_state,
                                                    view_ctx,
                                                    emitted_events,
                                                    rt);

      for (auto& event : emitted_events)
      {
        subject.emitted_events.push_back(event);
      }
    }

    void update_interaction(Runtime::View& view,
                            const Point& mouse_pos,
                            const MouseState& mouse_state)
    {
      int mx = mouse_pos.round_x();
      int my = mouse_pos.round_y();

      view.interaction.hovered = view.bounds.w > 0 && mx >= view.bounds.x &&
                                 mx < view.bounds.x + view.bounds.w && my >= view.bounds.y &&
                                 my < view.bounds.y + view.bounds.h;

      view.interaction.pressed.clear();
      for (auto& [btn, chain] : mouse_state.button_chains)
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

    void handle_mouse_up(MouseState& mouse_state,
                         FrameEvents& events,
                         Runtime::HookArguments& hook_args,
                         Lisple::Runtime& rt)
    {
      if (mouse_state.hovered_chain.empty()) return;

      const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
      auto chain = lock_chain(mouse_state.hovered_chain);
      if (chain.empty()) return;

      MouseButton up_btn = (events.mouse_button_up &&
                            events.mouse_button_up->type != Lisple::RTValue::Type::NIL)
                             ? mouse_button_from_name(events.mouse_button_up->str())
                             : MouseButton::NONE;

      {
        MouseButtonEvent ev;
        ev.global_pos = gp;
        ev.button = events.mouse_button_up;
        auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);
        bubble_hook(
          chain,
          &Runtime::Mode::on_mouse_up,
          ev_ref,
          ev.propagation_stopped,
          [&](const Rect& b) { ev.local_pos = local_pos(gp, b); },
          hook_args,
          rt);
      }

      auto pressed_view = mouse_state.pressed_by(up_btn);
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
          std::vector<std::shared_ptr<Runtime::View>> click_chain(it, chain.end());
          bubble_hook(
            click_chain,
            &Runtime::Mode::on_click,
            click_ev_ref,
            click_ev.propagation_stopped,
            [&](const Rect& b) { click_ev.local_pos = local_pos(gp, b); },
            hook_args,
            rt);
        }
      }
    }

    void handle_mouse_down(const std::shared_ptr<Runtime::View>& root,
                           MouseState& mouse_state,
                           FrameEvents& events,
                           Runtime::HookArguments& hook_args,
                           Lisple::Runtime& rt)
    {
      const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
      int mx = gp.round_x();
      int my = gp.round_y();

      std::vector<std::shared_ptr<Runtime::View>> hit_chain;
      if (!build_hit_chain(root, mx, my, hit_chain)) return;

      MouseButton btn = (events.mouse_button_down &&
                         events.mouse_button_down->type != Lisple::RTValue::Type::NIL)
                          ? mouse_button_from_name(events.mouse_button_down->str())
                          : MouseButton::NONE;
      auto& btn_chain = mouse_state.button_chains[btn];
      btn_chain.clear();
      for (auto& view_ptr : hit_chain)
      {
        btn_chain.push_back(std::weak_ptr<Runtime::View>(view_ptr));
        update_interaction(*view_ptr, gp, mouse_state);
      }

      MouseButtonEvent ev;
      ev.global_pos = gp;
      ev.button = events.mouse_button_down;
      auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);
      bubble_hook(
        hit_chain,
        &Runtime::Mode::on_mouse_down,
        ev_ref,
        ev.propagation_stopped,
        [&](const Rect& b) { ev.local_pos = local_pos(gp, b); },
        hook_args,
        rt);
    }

    void handle_mouse_motion(MouseState& mouse_state,
                             FrameEvents& events,
                             Runtime::HookArguments& hook_args,
                             Lisple::Runtime& rt)
    {
      auto chain = lock_chain(mouse_state.hovered_chain);
      if (chain.empty()) return;

      const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
      MouseEvent ev;
      ev.global_pos = gp;
      auto ev_ref = Script::MouseEventAdapter::make_ref(ev);
      bubble_hook(
        chain,
        &Runtime::Mode::on_mouse_motion,
        ev_ref,
        ev.propagation_stopped,
        [&](const Rect& b) { ev.local_pos = local_pos(gp, b); },
        hook_args,
        rt);
    }

    void traverse_child(const std::shared_ptr<Runtime::View>& view_ptr,
                        const MouseState& mouse_state,
                        Lisple::sptr_rtval* parent_state,
                        const Point& mouse_pos,
                        Runtime::HookArguments& hook_args,
                        Lisple::Runtime& rt)
    {
      auto& view = *view_ptr;

      if (parent_state)
      {
        view.state = Runtime::extract_state(*parent_state, view);
      }

      update_interaction(view, mouse_pos, mouse_state);
      run_update_hook(view_ptr, hook_args, rt);

      for (auto& child : view.children)
      {
        traverse_child(child, mouse_state, &view.state, mouse_pos, hook_args, rt);
        bubble_child_events_to_subject(view,
                                       parent_state,
                                       child,
                                       hook_args.update_args[1],
                                       rt);
      }

      if (parent_state)
      {
        *parent_state = Runtime::merge_state(*parent_state, view, view.state);
      }
    }

    void traverse(const std::shared_ptr<Runtime::View>& root,
                  MouseState& mouse_state,
                  FrameEvents& events,
                  Runtime::HookArguments& hook_args,
                  Lisple::Runtime& rt)
    {
      const Point& mouse_pos = Lisple::obj<Point>(*events.mouse_pos);
      int mx = mouse_pos.round_x();
      int my = mouse_pos.round_y();

      std::vector<std::shared_ptr<Runtime::View>> hit_chain;
      build_hit_chain(root, mx, my, hit_chain);
      std::shared_ptr<Runtime::View> new_hovered =
        hit_chain.empty() ? nullptr : hit_chain[0];

      auto old_hovered = mouse_state.hovered.lock();
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

          auto old_chain = lock_chain(mouse_state.hovered_chain);
          propagate_state_up_chain(old_chain);
        }

        mouse_state.hovered = new_hovered ? std::weak_ptr<Runtime::View>(new_hovered)
                                          : std::weak_ptr<Runtime::View>{};

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

      mouse_state.hovered_chain.clear();
      for (auto& v : hit_chain)
      {
        mouse_state.hovered_chain.push_back(std::weak_ptr<Runtime::View>(v));
      }

      traverse_child(root, mouse_state, nullptr, mouse_pos, hook_args, rt);
    }

  } // namespace

  void dispatch_interactions(const std::shared_ptr<Runtime::View>& root,
                             MouseState& mouse_state,
                             FrameEvents& events,
                             Runtime::HookArguments& hook_args,
                             Lisple::Runtime& runtime)
  {
    if (events.mouse_button_up && events.mouse_button_up->type != Lisple::RTValue::Type::NIL)
    {
      handle_mouse_up(mouse_state, events, hook_args, runtime);
    }

    if (events.mouse_button_down &&
        events.mouse_button_down->type != Lisple::RTValue::Type::NIL)
    {
      handle_mouse_down(root, mouse_state, events, hook_args, runtime);
    }

    traverse(root, mouse_state, events, hook_args, runtime);

    if (events.mouse_moved)
    {
      handle_mouse_motion(mouse_state, events, hook_args, runtime);
    }

    if (mouse_state.has_pressed() &&
        (!events.mouse_button_down ||
         events.mouse_button_down->type == Lisple::RTValue::Type::NIL))
    {
      std::set<MouseButton> held;
      if (events.mouse_held)
      {
        size_t n = Lisple::count(*events.mouse_held);
        for (size_t i = 0; i < n; i++)
        {
          held.insert(
            mouse_button_from_name(Lisple::get_child(*events.mouse_held, i)->str()));
        }
      }
      for (auto it = mouse_state.button_chains.begin();
           it != mouse_state.button_chains.end();)
      {
        if (!held.count(it->first))
        {
          it = mouse_state.button_chains.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }

} // namespace Pixils::UI
