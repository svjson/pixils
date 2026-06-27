#include <pixils/benchmark/counters.h>
#include <pixils/binding/ui/ui_namespace.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/event.h>
#include <pixils/ui/interaction_dispatch.h>
#include <pixils/ui/view_events.h>
#include <pixils/ui/view_update.h>

#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    Point update_mouse_pos(const HookArguments& hook_args)
    {
      if (!hook_args.events || !hook_args.events->mouse_pos ||
          hook_args.events->mouse_pos->type == Roo::Value::Type::NIL)
      {
        return {0.0f, 0.0f};
      }

      return Roo::obj<Point>(*hook_args.events->mouse_pos);
    }

  } // namespace

  void Session::update_mode()
  {
    PIXILS_BENCHMARK_COUNT(runtime_update_mode_calls);
    PIXILS_BENCHMARK_TIME_BLOCK(runtime_update_time_ns);

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

      Roo::sptr_val_v rargs = this->hook_args.update_args;
      auto ctx_parent_state = ctx.state;
      emitted_events = UI::process_view_events(ctx,
                                               &ctx_parent_state,
                                               nullptr,
                                               rargs.back(),
                                               emitted_events,
                                               roo_runtime);
      rargs[0] = ctx.state;
      if (ctx.mode->update && ctx.mode->update->type != Roo::Value::Type::NIL)
      {
        ctx.set_state_if_changed(
          invoke_hook(roo_runtime, view, ctx.mode->update, rargs, ctx.state));
      }
      mode_stack.update_state(ctx.state, update_stack.size() - i);

      ctx.drain_events(emitted_events);
    }

    if (hook_args.events)
    {
      Pixils::UI::dispatch_keyboard_events(active_mode,
                                           focus_state,
                                           *hook_args.events,
                                           hook_args,
                                           roo_runtime);
    }

    /**
     * Delegate active-mode update, hover tracking, and event dispatch to UI helpers.
     */
    if (hook_args.events)
    {
      bool late_interaction_update =
        Pixils::UI::dispatch_interactions(active_mode,
                                          mouse_state,
                                          focus_state,
                                          *hook_args.events,
                                          hook_args,
                                          roo_runtime);
      if (late_interaction_update)
      {
        Pixils::UI::update_view_tree(active_mode,
                                     mouse_state,
                                     focus_state,
                                     update_mouse_pos(hook_args),
                                     hook_args,
                                     roo_runtime);
      }
      Pixils::UI::sync_focus_state(active_mode, focus_state);
      Pixils::UI::refresh_view_interaction_tree(active_mode,
                                                mouse_state,
                                                focus_state,
                                                {0.0f, 0.0f});
    }
    this->hook_args.update_state(active_mode->state);
  }

} // namespace Pixils::Runtime
