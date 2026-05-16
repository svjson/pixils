#include "pixils/ui/interaction_dispatch.h"

#include "pixils/runtime/hook_arguments.h"
#include "pixils/runtime/hook_invocation.h"
#include "pixils/ui/event.h"
#include "pixils/ui/view_events.h"
#include "pixils/ui/view_lifecycle.h"
#include "pixils/ui/view_update.h"
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/ui/ui_namespace.h>
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
#include <optional>

namespace Pixils::UI
{
  namespace
  {
    Point local_pos(const Point& global, const Rect& bounds)
    {
      return {global.x - static_cast<float>(bounds.x),
              global.y - static_cast<float>(bounds.y)};
    }

    int scale_factor(const Style& style)
    {
      return std::max(1, style.scale.value_or(1));
    }

    Style interaction_style(const std::shared_ptr<Runtime::View>& view)
    {
      Style style = resolve_style(view->mode->style, view->state, view->interaction);
      if (view->effective_style.hidden) style.hidden = view->effective_style.hidden;
      if (view->effective_style.hit_test) style.hit_test = view->effective_style.hit_test;
      if (view->effective_style.clip) style.clip = view->effective_style.clip;
      if (view->effective_style.scale) style.scale = view->effective_style.scale;
      return style;
    }

    Rect external_bounds(const std::shared_ptr<Runtime::View>& view, const Style& style)
    {
      int scale = scale_factor(style);
      return {view->bounds.x,
              view->bounds.y,
              view->bounds.w * scale,
              view->bounds.h * scale};
    }

    Point to_logical_point(const std::shared_ptr<Runtime::View>& view,
                           const Style& style,
                           const Point& parent_point)
    {
      int scale = scale_factor(style);
      if (scale == 1) return parent_point;

      return {
        static_cast<float>(view->bounds.x) +
          (parent_point.x - static_cast<float>(view->bounds.x)) / static_cast<float>(scale),
        static_cast<float>(view->bounds.y) +
          (parent_point.y - static_cast<float>(view->bounds.y)) / static_cast<float>(scale)};
    }

    Point logical_pos_in_view(const Point& global,
                              const std::vector<std::shared_ptr<Runtime::View>>& chain,
                              size_t target_index)
    {
      Point p = global;
      for (size_t reverse_index = chain.size(); reverse_index > target_index;
           reverse_index--)
      {
        auto& view = chain[reverse_index - 1];
        p = to_logical_point(view, interaction_style(view), p);
      }
      return p;
    }

    Point local_pos_in_view(const Point& global,
                            const std::vector<std::shared_ptr<Runtime::View>>& chain,
                            size_t target_index)
    {
      Point logical = logical_pos_in_view(global, chain, target_index);
      return local_pos(logical, chain[target_index]->bounds);
    }

    bool has_drag_hooks(const std::shared_ptr<Runtime::View>& view)
    {
      return (view->mode->on_drag_start &&
              view->mode->on_drag_start->type != Lisple::Value::Type::NIL) ||
             (view->mode->on_drag &&
              view->mode->on_drag->type != Lisple::Value::Type::NIL) ||
             (view->mode->on_drag_end &&
              view->mode->on_drag_end->type != Lisple::Value::Type::NIL);
    }

    std::optional<std::pair<size_t, DragPolicy>> drag_policy_for_chain(
      const std::vector<std::shared_ptr<Runtime::View>>& chain,
      MouseButton button)
    {
      for (size_t i = 0; i < chain.size(); i++)
      {
        auto& view = chain[i];
        if (view->mode->drag)
        {
          if (view->mode->drag->button == button)
            return std::make_pair(i, *view->mode->drag);
          continue;
        }

        if (has_drag_hooks(view))
        {
          DragPolicy policy;
          policy.button = button;
          return std::make_pair(i, policy);
        }
      }

      return std::nullopt;
    }

    bool should_start_drag(const MouseState::DragState& drag_state, const Point& gp)
    {
      switch (drag_state.policy.start.mode)
      {
      case DragStartMode::IMMEDIATE:
        return true;
      case DragStartMode::THRESHOLD:
        return drag_state.start_global_pos.euclidean_distance_to(gp) >=
               static_cast<float>(drag_state.policy.start.distance);
      case DragStartMode::MOTION:
      default:
        return !(gp == drag_state.start_global_pos);
      }
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

    void store_focus_chain(FocusState& focus_state,
                           const std::vector<std::shared_ptr<Runtime::View>>& chain)
    {
      focus_state.clear();
      if (chain.empty()) return;

      focus_state.focused = chain[0];
      for (auto& view : chain)
      {
        focus_state.focus_chain.push_back(std::weak_ptr<Runtime::View>(view));
      }
    }

    Lisple::sptr_val resolve_callable_handler(Lisple::Runtime& runtime,
                                              const Lisple::sptr_val& val)
    {
      if (!val || val->type == Lisple::Value::Type::NIL) return Lisple::Constant::NIL;
      if (val->type == Lisple::Value::Type::SYMBOL)
      {
        return runtime.lookup(val->str());
      }
      if (val->type == Lisple::Value::Type::FUNCTION) return val;
      return Lisple::Constant::NIL;
    }

    void fire_hook_on_view(const std::shared_ptr<Runtime::View>& view,
                           const Lisple::sptr_val& hook,
                           const Lisple::sptr_val& ev_ref,
                           Runtime::HookArguments& hook_args,
                           Lisple::Runtime& rt)
    {
      if (!hook || hook->type == Lisple::Value::Type::NIL) return;
      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view;
      Lisple::sptr_val_v args = {view->state, ev_ref, hook_args.update_args[1]};
      auto new_state = Runtime::invoke_hook(rt, view, hook, args, view->state);
      if (new_state->type != Lisple::Value::Type::NIL)
      {
        view->state = new_state;
        for (auto& child : view->children)
        {
          restore_view_tree(child, view->state);
        }
      }
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
                     Lisple::sptr_val Runtime::Mode::* hook_field,
                     const Lisple::sptr_val& ev_ref,
                     bool& propagation_stopped,
                     const std::function<void(size_t)>& set_local_pos,
                     Runtime::HookArguments& hook_args,
                     Lisple::Runtime& rt)
    {
      for (size_t i = 0; i < chain.size(); i++)
      {
        auto& view = chain[i];
        if (!propagation_stopped)
        {
          set_local_pos(i);
          fire_hook_on_view(view, view->mode->*hook_field, ev_ref, hook_args, rt);
        }
        if (i + 1 < chain.size())
        {
          chain[i + 1]->state =
            Runtime::merge_state(chain[i + 1]->state, *view, view->state);
        }
      }
    }

    void bubble_emitted_events_from_chain(
      const std::vector<std::shared_ptr<Runtime::View>>& chain,
      Runtime::HookArguments& hook_args,
      Lisple::Runtime& rt)
    {
      auto view_ctx = hook_args.update_args[1];
      for (size_t source_index = 0; source_index < chain.size(); source_index++)
      {
        std::vector<CustomEvent> bubbled_events;
        chain[source_index]->drain_events(bubbled_events);
        if (bubbled_events.empty()) continue;

        if (source_index + 1 >= chain.size())
        {
          for (auto& event : bubbled_events)
          {
            chain[source_index]->emitted_events.push_back(event);
          }
          continue;
        }

        for (size_t receiver_index = source_index + 1;
             receiver_index < chain.size() && !bubbled_events.empty();
             receiver_index++)
        {
          auto parent_state =
            (receiver_index + 1 < chain.size()) ? &chain[receiver_index + 1]->state
                                                : nullptr;
          bubbled_events = process_view_events(*chain[receiver_index],
                                               parent_state,
                                               view_ctx,
                                               bubbled_events,
                                               rt,
                                               nullptr);
        }

        if (!bubbled_events.empty())
        {
          for (auto& event : bubbled_events)
          {
            chain.back()->emitted_events.push_back(event);
          }
        }
      }
    }

    void bubble_drag_hook(const std::vector<std::shared_ptr<Runtime::View>>& chain,
                          Lisple::sptr_val Runtime::Mode::* hook_field,
                          DragEvent& ev,
                          Runtime::HookArguments& hook_args,
                          Lisple::Runtime& rt)
    {
      auto ev_ref = Script::DragEventAdapter::make_ref(ev);
      bubble_hook(
        chain,
        hook_field,
        ev_ref,
        ev.propagation_stopped,
        [&](size_t index)
        {
          ev.local_pos = local_pos_in_view(ev.global_pos, chain, index);
          ev.start_local_pos = local_pos_in_view(ev.start_global_pos, chain, index);
        },
        hook_args,
        rt);
      bubble_emitted_events_from_chain(chain, hook_args, rt);
    }

    Lisple::sptr_val invoke_drag_payload_hook(const std::shared_ptr<Runtime::View>& source,
                                              const Lisple::sptr_val& hook,
                                              DragEvent& ev,
                                              Runtime::HookArguments& hook_args,
                                              Lisple::Runtime& rt)
    {
      auto payload_hook = resolve_callable_handler(rt, hook);
      if (!payload_hook || payload_hook->type == Lisple::Value::Type::NIL)
        return Lisple::Constant::NIL;

      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = source;
      auto ev_ref = Script::DragEventAdapter::make_ref(ev);
      Lisple::sptr_val_v args = {source->state, ev_ref, hook_args.update_args[1]};
      Lisple::Context exec_ctx(rt);
      auto result = payload_hook->exec().execute(exec_ctx, args);
      return result ? result : Lisple::Constant::NIL;
    }

    DragEvent make_drag_event(MouseButton button,
                              const Point& gp,
                              const MouseState::DragState& drag_state)
    {
      DragEvent ev;
      ev.global_pos = gp;
      ev.button = Lisple::keyword(mouse_button_name(button));
      ev.start_global_pos = drag_state.start_global_pos;
      ev.delta = gp - drag_state.last_global_pos;
      ev.total_delta = gp - drag_state.start_global_pos;
      ev.payload = Lisple::Constant::NIL;
      return ev;
    }

    void start_drag_operation(MouseState& mouse_state,
                              MouseButton button,
                              MouseState::DragState& drag_state,
                              const std::vector<std::shared_ptr<Runtime::View>>& chain,
                              const Point& gp,
                              Runtime::HookArguments& hook_args,
                              Lisple::Runtime& rt)
    {
      if (drag_state.source_index >= chain.size()) return;

      auto source = chain[drag_state.source_index];
      DragEvent drag_start_ev = make_drag_event(button, gp, drag_state);
      drag_start_ev.local_pos = local_pos_in_view(gp, chain, drag_state.source_index);
      drag_start_ev.start_local_pos =
        local_pos_in_view(drag_state.start_global_pos, chain, drag_state.source_index);
      drag_start_ev.payload = invoke_drag_payload_hook(source,
                                                       drag_state.policy.payload,
                                                       drag_start_ev,
                                                       hook_args,
                                                       rt);

      mouse_state.drag_operation = DragOperation{
        .button = button,
        .source = source,
        .start_global_pos = drag_state.start_global_pos,
        .current_global_pos = gp,
        .payload = drag_start_ev.payload,
        .policy = drag_state.policy,
      };

      bubble_drag_hook(chain, &Runtime::Mode::on_drag_start, drag_start_ev, hook_args, rt);
      drag_state.active = true;
      drag_state.last_global_pos = gp;
    }

    bool held_keys_contains(const Lisple::sptr_val& held_keys, const Lisple::sptr_val& key)
    {
      if (!held_keys || held_keys->type == Lisple::Value::Type::NIL || !key ||
          key->type == Lisple::Value::Type::NIL)
      {
        return false;
      }

      size_t held_count = Lisple::count(*held_keys);
      for (size_t i = 0; i < held_count; i++)
      {
        if (*Lisple::get_child(*held_keys, i) == *key) return true;
      }

      return false;
    }

    size_t key_spec_specificity(const Lisple::sptr_val& spec)
    {
      if (!spec) return 0;
      if (spec->type == Lisple::Value::Type::KEYWORD) return 1;
      if (spec->type == Lisple::Value::Type::VECTOR) return Lisple::count(*spec);
      return 0;
    }

    bool key_spec_matches_held(const Lisple::sptr_val& spec,
                               const Lisple::sptr_val& held_keys)
    {
      if (!spec || !held_keys || held_keys->type == Lisple::Value::Type::NIL) return false;

      if (spec->type == Lisple::Value::Type::KEYWORD)
      {
        return held_keys_contains(held_keys, spec);
      }

      if (spec->type != Lisple::Value::Type::VECTOR) return false;

      size_t key_count = Lisple::count(*spec);
      if (key_count == 0) return false;

      for (size_t i = 0; i < key_count; i++)
      {
        auto key = Lisple::get_child(*spec, i);
        if (!key || key->type != Lisple::Value::Type::KEYWORD) return false;
        if (!held_keys_contains(held_keys, key)) return false;
      }

      return true;
    }

    bool held_keys_contains_exact(const Lisple::sptr_val& held_keys,
                                  const std::string& key_name)
    {
      if (!held_keys || held_keys->type == Lisple::Value::Type::NIL) return false;

      size_t held_count = Lisple::count(*held_keys);
      for (size_t i = 0; i < held_count; i++)
      {
        auto held_key = Lisple::get_child(*held_keys, i);
        if (!held_key || held_key->type != Lisple::Value::Type::KEYWORD) continue;
        if (held_key->str() == key_name) return true;
      }

      return false;
    }

    std::string lower_ascii(std::string value)
    {
      std::transform(value.begin(),
                     value.end(),
                     value.begin(),
                     [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return value;
    }

    bool held_keys_has_shift(const Lisple::sptr_val& held_keys)
    {
      return held_keys_contains_exact(held_keys, "key/left-shift") ||
             held_keys_contains_exact(held_keys, "key/right-shift");
    }

    bool held_keys_has_ctrl(const Lisple::sptr_val& held_keys)
    {
      return held_keys_contains_exact(held_keys, "key/left-ctrl") ||
             held_keys_contains_exact(held_keys, "key/right-ctrl");
    }

    bool held_keys_has_alt(const Lisple::sptr_val& held_keys)
    {
      return held_keys_contains_exact(held_keys, "key/left-alt") ||
             held_keys_contains_exact(held_keys, "key/right-alt");
    }

    bool shortcut_matches_key_event(const Lisple::sptr_val& shortcut,
                                    const KeyboardEvent& event)
    {
      if (!shortcut || shortcut->type == Lisple::Value::Type::NIL || !event.key ||
          event.key->type != Lisple::Value::Type::KEYWORD)
      {
        return false;
      }

      bool expect_shift = false;
      bool expect_ctrl = false;
      bool expect_alt = false;
      Lisple::sptr_val primary = Lisple::Constant::NIL;

      auto consume_key = [&](const Lisple::sptr_val& key) -> bool
      {
        if (!key || key->type != Lisple::Value::Type::KEYWORD) return false;

        auto name = key->str();
        if (name == "key/shift" || name == "key/left-shift" || name == "key/right-shift")
        {
          expect_shift = true;
          return true;
        }
        if (name == "key/ctrl" || name == "key/left-ctrl" || name == "key/right-ctrl")
        {
          expect_ctrl = true;
          return true;
        }
        if (name == "key/alt" || name == "key/left-alt" || name == "key/right-alt")
        {
          expect_alt = true;
          return true;
        }
        if (primary && primary->type != Lisple::Value::Type::NIL)
        {
          return false;
        }

        primary = key;
        return true;
      };

      if (shortcut->type == Lisple::Value::Type::KEYWORD)
      {
        primary = shortcut;
      }
      else if (shortcut->type == Lisple::Value::Type::VECTOR)
      {
        size_t count = Lisple::count(*shortcut);
        for (size_t i = 0; i < count; i++)
        {
          if (!consume_key(Lisple::get_child(*shortcut, i))) return false;
        }
      }
      else
      {
        return false;
      }

      if (!primary || primary->type == Lisple::Value::Type::NIL) return false;
      if (lower_ascii(primary->str()) != lower_ascii(event.key->str())) return false;

      bool actual_shift = held_keys_has_shift(event.held_keys);
      bool actual_ctrl = held_keys_has_ctrl(event.held_keys);
      bool actual_alt = held_keys_has_alt(event.held_keys);

      return actual_shift == expect_shift && actual_ctrl == expect_ctrl &&
             actual_alt == expect_alt;
    }

    std::optional<CustomEvent> resolve_action_map_event(
      const std::shared_ptr<Runtime::View>& view,
      const KeyboardEvent& key_event)
    {
      if (!view || !view->mode || !view->mode->action_map ||
          view->mode->action_map->type == Lisple::Value::Type::NIL)
      {
        return std::nullopt;
      }

      auto try_binding = [&](const Lisple::sptr_val& shortcut,
                             const Lisple::sptr_val& binding) -> std::optional<CustomEvent>
      {
        Lisple::sptr_val action = Lisple::Constant::NIL;
        Lisple::sptr_val payload = Lisple::Constant::NIL;

        if (binding && binding->type == Lisple::Value::Type::KEYWORD)
        {
          action = binding;
        }
        else if (binding && binding->type == Lisple::Value::Type::MAP)
        {
          action = Lisple::Dict::get_property(binding, Lisple::keyword("action"));
          payload = Lisple::Dict::get_property(binding, Lisple::keyword("payload"));
        }

        if (!action || action->type != Lisple::Value::Type::KEYWORD) return std::nullopt;
        if (!shortcut_matches_key_event(shortcut, key_event)) return std::nullopt;

        return CustomEvent{
          action,
          payload && payload->type != Lisple::Value::Type::NIL ? payload
                                                               : Lisple::Constant::NIL,
          view->mode ? Lisple::symbol(view->mode->name) : Lisple::Constant::NIL};
      };

      auto action_map = view->mode->action_map;
      if (action_map->type != Lisple::Value::Type::MAP)
      {
        return std::nullopt;
      }

      for (const auto& shortcut : Lisple::Dict::keys(*action_map))
      {
        auto resolved =
          try_binding(shortcut, Lisple::Dict::get_property(action_map, shortcut));
        if (resolved.has_value()) return resolved;
      }

      return std::nullopt;
    }

    bool dispatch_action_map_event(const std::vector<std::shared_ptr<Runtime::View>>& chain,
                                   size_t start_index,
                                   const KeyboardEvent& key_event,
                                   Runtime::HookArguments& hook_args,
                                   Lisple::Runtime& rt)
    {
      for (size_t i = start_index; i < chain.size(); i++)
      {
        auto& view = chain[i];
        auto action_event = resolve_action_map_event(view, key_event);
        if (!action_event.has_value()) continue;

        std::vector<CustomEvent> events{*action_event};
        auto view_ctx = hook_args.update_args[1];
        auto bubbled_events =
          process_view_events(*view, nullptr, view_ctx, events, rt, nullptr);

        for (size_t j = i; j + 1 < chain.size(); j++)
        {
          chain[j + 1]->state =
            Runtime::merge_state(chain[j + 1]->state, *chain[j], chain[j]->state);

          if (!bubbled_events.empty())
          {
            auto parent_state = (j + 2 < chain.size()) ? &chain[j + 2]->state : nullptr;
            bubbled_events = process_view_events(*chain[j + 1],
                                                 parent_state,
                                                 view_ctx,
                                                 bubbled_events,
                                                 rt,
                                                 nullptr);
          }
        }

        return true;
      }

      return false;
    }

    std::vector<std::shared_ptr<Runtime::View>> keyboard_target_chain(
      const std::shared_ptr<Runtime::View>& root,
      const FocusState& focus_state)
    {
      auto focus_chain = lock_chain(focus_state.focus_chain);
      if (!focus_chain.empty())
      {
        return focus_chain;
      }

      if (!root)
      {
        return {};
      }

      return {root};
    }

    void bubble_keyboard_hook(const std::vector<std::shared_ptr<Runtime::View>>& chain,
                              Lisple::sptr_val Runtime::Mode::* hook_field,
                              KeyboardEvent& event,
                              Runtime::HookArguments& hook_args,
                              Lisple::Runtime& rt)
    {
      auto ev_ref = Script::KeyboardEventAdapter::make_ref(event);
      for (size_t i = 0; i < chain.size(); i++)
      {
        auto& view = chain[i];
        if (!event.propagation_stopped)
        {
          fire_hook_on_view(view, view->mode->*hook_field, ev_ref, hook_args, rt);
        }

        if (!event.propagation_stopped && hook_field == &Runtime::Mode::on_key_down)
        {
          if (dispatch_action_map_event(chain, i, event, hook_args, rt))
          {
            event.propagation_stopped = true;
          }
        }

        if (i + 1 < chain.size())
        {
          chain[i + 1]->state =
            Runtime::merge_state(chain[i + 1]->state, *view, view->state);
        }
      }
    }

    void bubble_held_key_hook(const std::vector<std::shared_ptr<Runtime::View>>& chain,
                              const Lisple::sptr_val& held_keys,
                              Runtime::HookArguments& hook_args,
                              Lisple::Runtime& rt)
    {
      if (!held_keys || held_keys->type == Lisple::Value::Type::NIL)
      {
        return;
      }

      for (size_t i = 0; i < chain.size(); i++)
      {
        auto& view = chain[i];
        auto& hook = view->mode->on_key_held;

        if (hook && hook->type != Lisple::Value::Type::NIL)
        {
          KeyboardEvent event;
          event.held_keys = held_keys;

          if (hook->type == Lisple::Value::Type::MAP)
          {
            std::vector<std::pair<Lisple::sptr_val, size_t>> matches;
            size_t best_specificity = 0;

            for (auto& spec : Lisple::Dict::keys(*hook))
            {
              if (!key_spec_matches_held(spec, held_keys)) continue;

              size_t specificity = key_spec_specificity(spec);
              if (specificity == 0) continue;

              if (specificity > best_specificity)
              {
                matches.clear();
                best_specificity = specificity;
              }

              if (specificity == best_specificity)
              {
                matches.emplace_back(spec, specificity);
              }
            }

            for (auto& [spec, _] : matches)
            {
              auto resolved_handler =
                resolve_callable_handler(rt, Lisple::Dict::get_property(hook, spec));
              if (!resolved_handler ||
                  resolved_handler->type != Lisple::Value::Type::FUNCTION)
              {
                continue;
              }

              event.match = spec;
              auto ev_ref = Script::KeyboardEventAdapter::make_ref(event);
              fire_hook_on_view(view, resolved_handler, ev_ref, hook_args, rt);
            }
          }
          else
          {
            auto resolved_handler = resolve_callable_handler(rt, hook);
            if (resolved_handler && resolved_handler->type == Lisple::Value::Type::FUNCTION)
            {
              auto ev_ref = Script::KeyboardEventAdapter::make_ref(event);
              fire_hook_on_view(view, resolved_handler, ev_ref, hook_args, rt);
            }
          }

          if (event.propagation_stopped)
          {
            if (i + 1 < chain.size())
            {
              chain[i + 1]->state =
                Runtime::merge_state(chain[i + 1]->state, *view, view->state);
            }
            return;
          }
        }

        if (i + 1 < chain.size())
        {
          chain[i + 1]->state =
            Runtime::merge_state(chain[i + 1]->state, *view, view->state);
        }
      }
    }

    bool build_hit_chain(std::shared_ptr<Runtime::View> view,
                         const Point& point,
                         std::vector<std::shared_ptr<Runtime::View>>& chain,
                         const std::optional<Rect>& inherited_clip = std::nullopt)
    {
      if (view->bounds.w == 0) return false;
      auto style = interaction_style(view);
      if (style.hidden && *style.hidden) return false;
      if (style.hit_test && !*style.hit_test) return false;
      if (inherited_clip &&
          (point.x < inherited_clip->x || point.x >= inherited_clip->x + inherited_clip->w ||
           point.y < inherited_clip->y || point.y >= inherited_clip->y + inherited_clip->h))
        return false;

      Rect hit_bounds = external_bounds(view, style);
      bool hit = point.x >= hit_bounds.x && point.x < hit_bounds.x + hit_bounds.w &&
                 point.y >= hit_bounds.y && point.y < hit_bounds.y + hit_bounds.h;
      if (!hit) return false;

      Point logical_point = to_logical_point(view, style, point);
      auto child_clip = inherited_clip;
      if (style.clip && *style.clip)
      {
        Rect content = style.content_rect(view->bounds);
        int x1 = child_clip ? std::max(child_clip->x, content.x) : content.x;
        int y1 = child_clip ? std::max(child_clip->y, content.y) : content.y;
        int x2 = child_clip ? std::min(child_clip->x + child_clip->w, content.x + content.w)
                            : content.x + content.w;
        int y2 = child_clip ? std::min(child_clip->y + child_clip->h, content.y + content.h)
                            : content.y + content.h;
        if (x2 > x1 && y2 > y1)
          child_clip = Rect{x1, y1, x2 - x1, y2 - y1};
        else
          child_clip = std::nullopt;
      }

      bool visit_children = true;
      if (style.clip && *style.clip)
      {
        visit_children = child_clip && logical_point.x >= child_clip->x &&
                         logical_point.x < child_clip->x + child_clip->w &&
                         logical_point.y >= child_clip->y &&
                         logical_point.y < child_clip->y + child_clip->h;
      }

      for (auto it = view->children.rbegin(); visit_children && it != view->children.rend();
           ++it)
      {
        if (build_hit_chain(*it, logical_point, chain, child_clip))
        {
          chain.push_back(view);
          return true;
        }
      }

      chain.push_back(view);
      return true;
    }

    bool is_focusable(const std::shared_ptr<Runtime::View>& view)
    {
      return view && view->mode && view->mode->focusable;
    }

    bool find_focus_chain(const std::shared_ptr<Runtime::View>& view,
                          Runtime::View* target,
                          std::vector<std::shared_ptr<Runtime::View>>& chain)
    {
      if (!view || !target) return false;

      auto style = resolve_style(view->mode->style, view->state, view->interaction);
      if (style.hidden && *style.hidden) return false;

      if (view.get() == target)
      {
        chain.push_back(view);
        return true;
      }

      for (auto& child : view->children)
      {
        if (find_focus_chain(child, target, chain))
        {
          chain.push_back(view);
          return true;
        }
      }

      return false;
    }

    bool restore_nearest_focusable_ancestor(const std::shared_ptr<Runtime::View>& root,
                                            FocusState& focus_state)
    {
      if (!root) return false;

      auto previous_chain = focus_state.focus_chain;
      for (size_t i = 1; i < previous_chain.size(); i++)
      {
        auto candidate = previous_chain[i].lock();
        if (!is_focusable(candidate)) continue;

        std::vector<std::shared_ptr<Runtime::View>> chain;
        if (find_focus_chain(root, candidate.get(), chain))
        {
          store_focus_chain(focus_state, chain);
          return true;
        }
      }

      return false;
    }

    void sync_focus_state_impl(const std::shared_ptr<Runtime::View>& root,
                               FocusState& focus_state)
    {
      auto focused = focus_state.focused.lock();
      if (!root)
      {
        focus_state.clear();
        return;
      }

      if (!focused)
      {
        if (!restore_nearest_focusable_ancestor(root, focus_state))
        {
          focus_state.clear();
        }
        return;
      }

      std::vector<std::shared_ptr<Runtime::View>> chain;
      if (!find_focus_chain(root, focused.get(), chain))
      {
        if (!restore_nearest_focusable_ancestor(root, focus_state))
        {
          focus_state.clear();
        }
        return;
      }

      store_focus_chain(focus_state, chain);
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

      MouseButton up_btn =
        (events.mouse_button_up && events.mouse_button_up->type != Lisple::Value::Type::NIL)
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
          [&](size_t index) { ev.local_pos = local_pos_in_view(gp, chain, index); },
          hook_args,
          rt);
      }

      bool drag_active = false;
      auto drag_it = mouse_state.drag_states.find(up_btn);
      if (drag_it != mouse_state.drag_states.end())
      {
        drag_active = drag_it->second.active;
        auto pressed_chain_it = mouse_state.button_chains.find(up_btn);
        if (drag_active && pressed_chain_it != mouse_state.button_chains.end())
        {
          auto pressed_chain = lock_chain(pressed_chain_it->second);
          if (!pressed_chain.empty())
          {
            DragEvent drag_end_ev = make_drag_event(up_btn, gp, drag_it->second);
            drag_end_ev.button = events.mouse_button_up;
            if (mouse_state.drag_operation)
            {
              mouse_state.drag_operation->current_global_pos = gp;
              drag_end_ev.payload = mouse_state.drag_operation->payload;
            }
            bubble_drag_hook(pressed_chain,
                             &Runtime::Mode::on_drag_end,
                             drag_end_ev,
                             hook_args,
                             rt);
          }

          DragEvent drop_ev = make_drag_event(up_btn, gp, drag_it->second);
          drop_ev.button = events.mouse_button_up;
          if (mouse_state.drag_operation)
            drop_ev.payload = mouse_state.drag_operation->payload;
          bubble_drag_hook(chain, &Runtime::Mode::on_drop, drop_ev, hook_args, rt);
          mouse_state.drag_operation = std::nullopt;
        }
      }

      auto pressed_view = mouse_state.pressed_by(up_btn);
      if (pressed_view && !drag_active)
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
            [&](size_t index)
            { click_ev.local_pos = local_pos_in_view(gp, click_chain, index); },
            hook_args,
            rt);
        }
      }
    }

    void handle_mouse_down(const std::shared_ptr<Runtime::View>& root,
                           MouseState& mouse_state,
                           FocusState& focus_state,
                           FrameEvents& events,
                           Runtime::HookArguments& hook_args,
                           Lisple::Runtime& rt)
    {
      const Point& gp = Lisple::obj<Point>(*events.mouse_pos);

      std::vector<std::shared_ptr<Runtime::View>> hit_chain;
      if (!build_hit_chain(root, gp, hit_chain))
      {
        focus_state.clear();
        return;
      }

      auto focus_it = std::find_if(hit_chain.begin(),
                                   hit_chain.end(),
                                   [](const auto& view) { return is_focusable(view); });

      if (focus_it == hit_chain.end())
      {
        focus_state.clear();
      }
      else
      {
        std::vector<std::shared_ptr<Runtime::View>> focus_chain(focus_it, hit_chain.end());
        store_focus_chain(focus_state, focus_chain);
      }

      MouseButton btn = (events.mouse_button_down &&
                         events.mouse_button_down->type != Lisple::Value::Type::NIL)
                          ? mouse_button_from_name(events.mouse_button_down->str())
                          : MouseButton::NONE;
      auto& btn_chain = mouse_state.button_chains[btn];
      btn_chain.clear();
      for (auto& view_ptr : hit_chain)
      {
        btn_chain.push_back(std::weak_ptr<Runtime::View>(view_ptr));
      }
      auto drag_policy = drag_policy_for_chain(hit_chain, btn);
      mouse_state.drag_states[btn] = MouseState::DragState{
        .start_global_pos = gp,
        .last_global_pos = gp,
        .eligible = drag_policy.has_value(),
        .active = false,
        .source_index = drag_policy ? drag_policy->first : 0,
        .policy = drag_policy ? drag_policy->second : DragPolicy{},
      };

      MouseButtonEvent ev;
      ev.global_pos = gp;
      ev.button = events.mouse_button_down;
      auto ev_ref = Script::MouseButtonEventAdapter::make_ref(ev);
      bubble_hook(
        hit_chain,
        &Runtime::Mode::on_mouse_down,
        ev_ref,
        ev.propagation_stopped,
        [&](size_t index) { ev.local_pos = local_pos_in_view(gp, hit_chain, index); },
        hook_args,
        rt);

      auto drag_it = mouse_state.drag_states.find(btn);
      if (drag_it != mouse_state.drag_states.end() && drag_it->second.eligible &&
          drag_it->second.policy.start.mode == DragStartMode::IMMEDIATE)
      {
        start_drag_operation(mouse_state,
                             btn,
                             drag_it->second,
                             hit_chain,
                             gp,
                             hook_args,
                             rt);
      }
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
        [&](size_t index) { ev.local_pos = local_pos_in_view(gp, chain, index); },
        hook_args,
        rt);
    }

    void handle_drag_motion(MouseState& mouse_state,
                            FrameEvents& events,
                            Runtime::HookArguments& hook_args,
                            Lisple::Runtime& rt)
    {
      if (!events.mouse_moved) return;

      const Point& gp = Lisple::obj<Point>(*events.mouse_pos);
      for (auto& [btn, wchain] : mouse_state.button_chains)
      {
        auto drag_it = mouse_state.drag_states.find(btn);
        if (drag_it == mouse_state.drag_states.end() || !drag_it->second.eligible) continue;

        auto chain = lock_chain(wchain);
        if (chain.empty()) continue;

        auto& drag_state = drag_it->second;
        if (!drag_state.active)
        {
          if (!should_start_drag(drag_state, gp)) continue;
          start_drag_operation(mouse_state, btn, drag_state, chain, gp, hook_args, rt);
          continue;
        }

        if (gp == drag_state.last_global_pos) continue;

        DragEvent drag_ev = make_drag_event(btn, gp, drag_state);
        if (mouse_state.drag_operation)
        {
          mouse_state.drag_operation->current_global_pos = gp;
          drag_ev.payload = mouse_state.drag_operation->payload;
        }
        bubble_drag_hook(chain, &Runtime::Mode::on_drag, drag_ev, hook_args, rt);
        drag_state.last_global_pos = gp;
      }
    }

    void traverse(const std::shared_ptr<Runtime::View>& root,
                  MouseState& mouse_state,
                  const FocusState& focus_state,
                  FrameEvents& events,
                  Runtime::HookArguments& hook_args,
                  Lisple::Runtime& rt)
    {
      const Point& mouse_pos = Lisple::obj<Point>(*events.mouse_pos);

      std::vector<std::shared_ptr<Runtime::View>> hit_chain;
      build_hit_chain(root, mouse_pos, hit_chain);
      std::shared_ptr<Runtime::View> new_hovered =
        hit_chain.empty() ? nullptr : hit_chain[0];

      auto old_hovered = mouse_state.hovered.lock();
      if (old_hovered.get() != new_hovered.get())
      {
        if (old_hovered)
        {
          auto old_chain = lock_chain(mouse_state.hovered_chain);
          MouseEvent leave_ev;
          leave_ev.global_pos = mouse_pos;
          leave_ev.local_pos = old_chain.empty()
                                 ? local_pos(mouse_pos, old_hovered->bounds)
                                 : local_pos_in_view(mouse_pos, old_chain, 0);
          auto ev_ref = Script::MouseEventAdapter::make_ref(leave_ev);
          fire_hook_on_view(old_hovered,
                            old_hovered->mode->on_mouse_leave,
                            ev_ref,
                            hook_args,
                            rt);

          propagate_state_up_chain(old_chain);
        }

        mouse_state.hovered = new_hovered ? std::weak_ptr<Runtime::View>(new_hovered)
                                          : std::weak_ptr<Runtime::View>{};

        if (new_hovered)
        {
          MouseEvent enter_ev;
          enter_ev.global_pos = mouse_pos;
          enter_ev.local_pos = local_pos_in_view(mouse_pos, hit_chain, 0);
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

      update_view_tree(root, mouse_state, focus_state, mouse_pos, hook_args, rt);
    }

  } // namespace

  void sync_focus_state(const std::shared_ptr<Runtime::View>& root,
                        FocusState& focus_state)
  {
    sync_focus_state_impl(root, focus_state);
  }

  void dispatch_keyboard_events(const std::shared_ptr<Runtime::View>& root,
                                FocusState& focus_state,
                                FrameEvents& events,
                                Runtime::HookArguments& hook_args,
                                Lisple::Runtime& runtime)
  {
    sync_focus_state_impl(root, focus_state);
    auto chain = keyboard_target_chain(root, focus_state);
    if (chain.empty())
    {
      return;
    }

    if (events.key_down && events.key_down->type != Lisple::Value::Type::NIL)
    {
      KeyboardEvent event;
      event.key = events.key_down;
      event.held_keys = events.held_keys;
      bubble_keyboard_hook(chain, &Runtime::Mode::on_key_down, event, hook_args, runtime);
    }

    bubble_held_key_hook(chain, events.held_keys, hook_args, runtime);

    if (events.key_up && events.key_up->type != Lisple::Value::Type::NIL)
    {
      KeyboardEvent event;
      event.key = events.key_up;
      event.held_keys = events.held_keys;
      bubble_keyboard_hook(chain, &Runtime::Mode::on_key_up, event, hook_args, runtime);
    }
  }

  void dispatch_interactions(const std::shared_ptr<Runtime::View>& root,
                             MouseState& mouse_state,
                             FocusState& focus_state,
                             FrameEvents& events,
                             Runtime::HookArguments& hook_args,
                             Lisple::Runtime& runtime)
  {
    sync_focus_state_impl(root, focus_state);

    if (events.mouse_button_up && events.mouse_button_up->type != Lisple::Value::Type::NIL)
    {
      handle_mouse_up(mouse_state, events, hook_args, runtime);
    }

    if (events.mouse_button_down &&
        events.mouse_button_down->type != Lisple::Value::Type::NIL)
    {
      handle_mouse_down(root, mouse_state, focus_state, events, hook_args, runtime);
    }

    sync_focus_state_impl(root, focus_state);
    traverse(root, mouse_state, focus_state, events, hook_args, runtime);

    if (events.mouse_moved)
    {
      handle_mouse_motion(mouse_state, events, hook_args, runtime);
      handle_drag_motion(mouse_state, events, hook_args, runtime);
    }

    if (mouse_state.has_pressed() &&
        (!events.mouse_button_down ||
         events.mouse_button_down->type == Lisple::Value::Type::NIL))
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
          if (mouse_state.drag_operation && mouse_state.drag_operation->button == it->first)
            mouse_state.drag_operation = std::nullopt;
          mouse_state.drag_states.erase(it->first);
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
