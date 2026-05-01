#include "pixils/runtime/session.h"
#include "pixils/runtime/view.h"
#include "pixils/ui/event.h"
#include "pixils/ui/interaction_dispatch.h"
#include "pixils/ui/view_events.h"

#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
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
      ctx.state = invoke_hook(view, ctx.mode->update, rargs, ctx.state);
      mode_stack.update_state(ctx.state, update_stack.size() - i);

      ctx.drain_events(emitted_events);
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
