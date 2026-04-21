
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>

#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace
{
  /**
   * Extract a view's state slice from the parent's state map at the given
   * keyword id. Returns NIL if parent is not a map or the key is absent.
   */
  Lisple::sptr_rtval extract_child_state(const Lisple::sptr_rtval& parent,
                                         const std::string& id)
  {
    if (!parent || parent->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
    auto val = Lisple::Dict::get_property(parent, Lisple::RTValue::keyword(id));
    return val ? val : Lisple::Constant::NIL;
  }

  /**
   * Store child_state into parent's state map under keyword id, returning the
   * updated parent. Creates a new empty map if parent is NIL.
   */
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

} // namespace

namespace Pixils::Runtime
{

  Lisple::sptr_rtval Session::init_view(View& ctx,
                                        const Lisple::sptr_rtval& parent_state)
  {
    if (!this->assets.is_loaded(ctx.mode->name))
      this->assets.load(ctx.mode->name, ctx.mode->resources);

    ctx.state = extract_child_state(parent_state, ctx.id);
    if (ctx.state->type == Lisple::RTValue::Type::NIL) ctx.state = ctx.initial_state;

    Lisple::sptr_rtval_v iargs = {ctx.state, this->hook_args.init_args[1]};
    auto new_state = invoke_hook(ctx.mode->init, iargs);
    if (new_state->type != Lisple::RTValue::Type::NIL) ctx.state = new_state;

    for (auto& grandchild : ctx.children)
      ctx.state = init_view(*grandchild, ctx.state);

    return merge_child_state(parent_state, ctx.id, ctx.state);
  }

  void Session::restore_view_state(View& ctx, const Lisple::sptr_rtval& parent_state)
  {
    ctx.state = extract_child_state(parent_state, ctx.id);
    for (auto& grandchild : ctx.children)
      restore_view_state(*grandchild, ctx.state);
  }

  void Session::update_mode()
  {
    auto update_stack = mode_stack.get_update_stack();

    /**
     * Update composition modes below the top, preserving the existing offset semantics.
     */
    for (size_t i = update_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      View& ctx = *ctx_stack[ctx_idx];

      Lisple::sptr_rtval_v rargs = this->hook_args.update_args;
      rargs[0] = ctx.state;
      ctx.state = invoke_hook(ctx.mode->update, rargs, ctx.state);
      mode_stack.update_state(ctx.state, update_stack.size() - i);
    }

    /**
     * Delegate active-mode update, hover tracking, and event dispatch to EventRouter.
     */
    if (hook_args.events)
      event_router.update(active_mode, *hook_args.events, hook_args, lisple_runtime);
    this->hook_args.update_state(active_mode->state);
  }

} // namespace Pixils::Runtime
