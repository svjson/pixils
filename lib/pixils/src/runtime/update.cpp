
#include "pixils/runtime/event_routing.h"
#include "pixils/runtime/session.h"
#include "pixils/ui/event.h"
#include <pixils/asset/registry.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>

#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  Lisple::sptr_rtval Session::init_view(View& ctx, const Lisple::sptr_rtval& parent_state)
  {
    if (!this->assets.is_loaded(ctx.mode->name))
      this->assets.load(ctx.mode->name, ctx.mode->resources);

    ctx.state = extract_state(parent_state, ctx);

    Lisple::sptr_rtval_v iargs = {ctx.state, this->hook_args.init_args[1]};
    auto new_state = invoke_hook(ctx.mode->init, iargs);
    if (new_state->type != Lisple::RTValue::Type::NIL) ctx.state = new_state;

    for (auto& grandchild : ctx.children)
      ctx.state = init_view(*grandchild, ctx.state);

    return merge_state(parent_state, ctx, ctx.state);
  }

  void Session::restore_view_state(View& ctx, const Lisple::sptr_rtval& parent_state)
  {
    ctx.state = extract_state(parent_state, ctx);
    for (auto& grandchild : ctx.children)
      restore_view_state(*grandchild, ctx.state);
  }

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
      View& ctx = *ctx_stack[ctx_idx];

      Lisple::sptr_rtval_v rargs = this->hook_args.update_args;
      emitted_events =
        EventRouter::process_events(ctx, rargs.back(), emitted_events, lisple_runtime);
      rargs[0] = ctx.state;
      ctx.state = invoke_hook(ctx.mode->update, rargs, ctx.state);
      mode_stack.update_state(ctx.state, update_stack.size() - i);

      ctx.drain_events(emitted_events);
    }

    /**
     * Delegate active-mode update, hover tracking, and event dispatch to EventRouter.
     */
    if (hook_args.events)
      event_router.update(active_mode, *hook_args.events, hook_args, lisple_runtime);
    this->hook_args.update_state(active_mode->state);
  }

} // namespace Pixils::Runtime
