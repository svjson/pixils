#include "pixils/runtime/event_routing.h"
#include "pixils/runtime/session.h"
#include "pixils/runtime/view.h"
#include "pixils/ui/event.h"

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
      emitted_events = EventRouter::process_events(ctx,
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
     * Delegate active-mode update, hover tracking, and event dispatch to EventRouter.
     */
    if (hook_args.events)
      event_router.update(active_mode,
                          mouse_state,
                          *hook_args.events,
                          hook_args,
                          lisple_runtime);
    this->hook_args.update_state(active_mode->state);
  }

} // namespace Pixils::Runtime
