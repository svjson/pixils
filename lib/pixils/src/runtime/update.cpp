#include "pixils/binding/ui_namespace.h"
#include "pixils/runtime/hook_invocation.h"
#include "pixils/runtime/session.h"
#include "pixils/runtime/view.h"
#include "pixils/ui/event.h"
#include "pixils/ui/interaction_dispatch.h"
#include "pixils/ui/view_events.h"

#include <algorithm>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    Lisple::sptr_rtval resolve_callable_handler(Lisple::Runtime& runtime,
                                                const Lisple::sptr_rtval& val)
    {
      if (!val || val->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
      if (val->type == Lisple::RTValue::Type::SYMBOL)
        return runtime.lookup_value(val->str());
      if (val->type == Lisple::RTValue::Type::FUNCTION) return val;
      return Lisple::Constant::NIL;
    }

    void invoke_root_keyboard_hook(const std::shared_ptr<View>& view,
                                   const Lisple::sptr_rtval& hook,
                                   KeyboardEvent& event,
                                   HookArguments& hook_args,
                                   Lisple::Runtime& runtime)
    {
      if (!view || !hook || hook->type == Lisple::RTValue::Type::NIL)
      {
        return;
      }

      auto event_ref = Script::KeyboardEventAdapter::make_ref(event);
      Lisple::sptr_rtval_v args = {view->state, event_ref, hook_args.update_args[1]};
      view->state = invoke_hook(runtime, view, hook, args, view->state);
    }

    void run_root_key_hook(const std::shared_ptr<View>& view,
                           const Lisple::sptr_rtval& hook,
                           const Lisple::sptr_rtval& key,
                           HookArguments& hook_args,
                           Lisple::Runtime& runtime)
    {
      if (!key || key->type == Lisple::RTValue::Type::NIL)
      {
        return;
      }

      KeyboardEvent event;
      event.key = key;
      invoke_root_keyboard_hook(view, hook, event, hook_args, runtime);
    }

    bool held_keys_contains(const Lisple::sptr_rtval& held_keys,
                            const Lisple::sptr_rtval& key)
    {
      if (!held_keys || held_keys->type == Lisple::RTValue::Type::NIL || !key ||
          key->type == Lisple::RTValue::Type::NIL)
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

    size_t key_spec_specificity(const Lisple::sptr_rtval& spec)
    {
      if (!spec) return 0;
      if (spec->type == Lisple::RTValue::Type::KEYWORD) return 1;
      if (spec->type == Lisple::RTValue::Type::VECTOR) return Lisple::count(*spec);
      return 0;
    }

    bool key_spec_matches_held(const Lisple::sptr_rtval& spec,
                               const Lisple::sptr_rtval& held_keys)
    {
      if (!spec || !held_keys || held_keys->type == Lisple::RTValue::Type::NIL) return false;

      if (spec->type == Lisple::RTValue::Type::KEYWORD)
      {
        return held_keys_contains(held_keys, spec);
      }

      if (spec->type != Lisple::RTValue::Type::VECTOR) return false;

      size_t key_count = Lisple::count(*spec);
      if (key_count == 0) return false;

      for (size_t i = 0; i < key_count; i++)
      {
        auto key = Lisple::get_child(*spec, i);
        if (!key || key->type != Lisple::RTValue::Type::KEYWORD) return false;
        if (!held_keys_contains(held_keys, key)) return false;
      }

      return true;
    }

    void run_root_held_key_hook(const std::shared_ptr<View>& view,
                                const Lisple::sptr_rtval& hook,
                                const Lisple::sptr_rtval& held_keys,
                                HookArguments& hook_args,
                                Lisple::Runtime& runtime)
    {
      if (!view || !hook || hook->type == Lisple::RTValue::Type::NIL || !held_keys ||
          held_keys->type == Lisple::RTValue::Type::NIL)
      {
        return;
      }

      if (hook->type == Lisple::RTValue::Type::MAP)
      {
        std::vector<std::pair<Lisple::sptr_rtval, size_t>> matches;
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
            resolve_callable_handler(runtime, Lisple::Dict::get_property(hook, spec));
          if (!resolved_handler || resolved_handler->type != Lisple::RTValue::Type::FUNCTION)
            continue;

          KeyboardEvent event;
          event.held_keys = held_keys;
          event.match = spec;
          invoke_root_keyboard_hook(view, resolved_handler, event, hook_args, runtime);
        }

        return;
      }

      auto resolved_handler = resolve_callable_handler(runtime, hook);
      if (!resolved_handler || resolved_handler->type != Lisple::RTValue::Type::FUNCTION)
        return;

      KeyboardEvent event;
      event.held_keys = held_keys;
      invoke_root_keyboard_hook(view, resolved_handler, event, hook_args, runtime);
    }
  } // namespace

  void Session::update_mode()
  {
    auto update_stack = mode_stack.get_update_stack();

    std::vector<CustomEvent> emitted_events;

    /**
     * Update composition modes below the top, preserving the existing offset semantics.
     */
    for (size_t i = update_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      auto view = ctx_stack[ctx_idx];
      View& ctx = *view;

      Lisple::sptr_rtval_v rargs = this->hook_args.update_args;
      auto ctx_parent_state = ctx.state;
      emitted_events = UI::process_view_events(ctx,
                                               &ctx_parent_state,
                                               rargs.back(),
                                               emitted_events,
                                               lisple_runtime);
      rargs[0] = ctx.state;
      ctx.state = invoke_hook(lisple_runtime, view, ctx.mode->update, rargs, ctx.state);
      mode_stack.update_state(ctx.state, update_stack.size() - i);

      ctx.drain_events(emitted_events);
    }

    if (hook_args.events)
    {
      run_root_key_hook(active_mode,
                        active_mode->mode->on_key_down,
                        hook_args.events->key_down,
                        hook_args,
                        lisple_runtime);
      run_root_held_key_hook(active_mode,
                             active_mode->mode->on_key_held,
                             hook_args.events->held_keys,
                             hook_args,
                             lisple_runtime);
      run_root_key_hook(active_mode,
                        active_mode->mode->on_key_up,
                        hook_args.events->key_up,
                        hook_args,
                        lisple_runtime);
    }

    /**
     * Delegate active-mode update, hover tracking, and event dispatch to UI helpers.
     */
    if (hook_args.events)
      Pixils::UI::dispatch_interactions(active_mode,
                                        mouse_state,
                                        *hook_args.events,
                                        hook_args,
                                        lisple_runtime);
    this->hook_args.update_state(active_mode->state);
  }

} // namespace Pixils::Runtime
