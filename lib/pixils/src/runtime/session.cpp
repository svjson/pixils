
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/style_namespace.h>
#include <pixils/binding/ui_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_lifecycle.h>
#include <pixils/ui/view_render.h>

#include <SDL2/SDL_render.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  Session::Session(Lisple::Runtime& lisple_runtime,
                   Asset::Registry& assets,
                   RenderContext& render_ctx,
                   const HookArguments& hook_args)
    : lisple_runtime(lisple_runtime)
    , assets(assets)
    , render_ctx(render_ctx)
    , mode_stack(lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK),
                 lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK_MESSAGES))
    , modes(lisple_runtime.lookup_value(Script::ID__PIXILS__MODES))
    , hook_args(hook_args)
  {
  }

  void Session::pop_mode()
  {
    if (mode_stack.size() > 1)
    {
      mode_stack.pop();

      /**
       * Restore active_mode from the context stack. Then re-sync
       * state from the Lisple stack (which holds the authoritative
       * saved state) and restore each child's state slice from that
       * parent state.
       */
      active_mode = ctx_stack.back();
      ctx_stack.pop_back();

      auto [_, saved_state] = mode_stack.peek();
      active_mode->state = saved_state;
      for (auto& child : active_mode->children)
      {
        Pixils::UI::restore_view_tree(child, active_mode->state);
      }

      this->hook_args.update_state(active_mode->state);
    }
  }

  void Session::push_mode(const Lisple::sptr_rtval& mode,
                          const Lisple::sptr_rtval& state,
                          const Lisple::sptr_rtval& overrides)
  {
    /**
     * Flush the current active context's state to the Lisple stack before pushing,
     * then save the context itself so pop_mode can recover it cheaply.
     */
    if (mode_stack.size() > 0)
    {
      mode_stack.update_state(active_mode->state);
      ctx_stack.push_back(std::move(active_mode));
    }
    this->mode_stack.push(mode, state);

    auto& mode_obj = Lisple::obj<Mode>(*mode);

    active_mode = Pixils::UI::build_root_view(mode_obj, state, overrides, lisple_runtime);

    this->hook_args.update_state(state);

    Pixils::UI::init_root_view(assets, lisple_runtime, hook_args.init_args[1], active_mode);
    this->hook_args.update_state(active_mode->state);

    /**
     * Build children and initialize each child, threading child states into
     * the parent state map as they complete.
     */
    auto parent_state = this->active_mode->state;
    for (const auto& slot : active_mode->mode->children)
    {
      this->active_mode->children.push_back(
        Pixils::UI::build_view_tree(slot, modes, lisple_runtime));
      parent_state = Pixils::UI::init_view_tree(assets,
                                                lisple_runtime,
                                                hook_args.init_args[1],
                                                this->active_mode->children.back(),
                                                parent_state);
    }

    this->active_mode->state = parent_state;
    this->hook_args.update_state(parent_state);
  }

  void Session::push_mode(const std::string& mode_name,
                          const Lisple::sptr_rtval& state,
                          const Lisple::sptr_rtval& overrides)
  {
    auto mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(mode_name));
    this->push_mode(mode, state, overrides);
  }

  void Session::process_messages()
  {
    Lisple::sptr_rtval_v messages = mode_stack.drain_messages();

    for (auto& message : messages)
    {
      std::string type =
        Lisple::Dict::get_property(message, Lisple::RTValue::keyword("type"))->str();

      if (type == "push")
      {
        mode_stack.update_state(active_mode->state);
        auto overrides_val =
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("overrides"));
        push_mode(
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("mode"))->str(),
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("state")),
          overrides_val ? overrides_val : Lisple::Constant::NIL);
      }
      else if (type == "pop")
      {
        pop_mode();
      }
    }
  }

  void Session::render_mode()
  {
    auto render_stack = mode_stack.get_render_stack();
    Rect full = {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h};

    /**
     * render_stack is top-first: [0]=top, [1]=just below, ..., [n-1]=bottom.
     * Render from bottom up, skipping index 0 (active_mode, rendered last).
     */
    for (size_t i = render_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      Pixils::UI::render_view(render_ctx,
                              lisple_runtime,
                              hook_args.render_args[1],
                              ctx_stack[ctx_idx],
                              full);
    }

    Pixils::UI::render_view(render_ctx,
                            lisple_runtime,
                            hook_args.render_args[1],
                            active_mode,
                            full);
  }
} // namespace Pixils::Runtime
