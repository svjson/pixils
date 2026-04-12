
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>

#include <SDL2/SDL_render.h>
#include <lisple/context.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace
{
  /**
   * Resolve a hook value at mode activation time. Symbols are looked up once
   * and replaced with the callable they name; callables and NIL pass through.
   * A null (absent) value is treated as NIL.
   */
  Lisple::sptr_rtval resolve_hook(Lisple::Runtime& runtime, const Lisple::sptr_rtval& val)
  {
    if (!val || val->type == Lisple::RTValue::Type::NIL) return Lisple::Constant::NIL;
    if (val->type == Lisple::RTValue::Type::SYMBOL) return runtime.lookup_value(val->str());
    if (val->type == Lisple::RTValue::Type::FUNCTION) return val;
    return Lisple::Constant::NIL;
  }

  /**
   * Invoke a pre-resolved hook. Returns fallback if the hook is NIL (absent).
   * Hooks must already be resolved - no symbol lookup is performed here.
   */
  Lisple::sptr_rtval invoke_hook(Lisple::Runtime& runtime,
                                 const Lisple::sptr_rtval& fn,
                                 Lisple::sptr_rtval_v& args,
                                 const Lisple::sptr_rtval& fallback = Lisple::Constant::NIL)
  {
    if (!fn || fn->type == Lisple::RTValue::Type::NIL) return fallback;
    Lisple::Context exec_ctx(runtime);
    return fn->exec().execute(exec_ctx, args);
  }
} // namespace

namespace Pixils::Runtime
{
  void HookArguments::update_state(const Lisple::sptr_rtval& state)
  {
    this->init_args[0] = state;
    this->update_args[0] = state;
    this->render_args[0] = state;
  }

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

      auto [mode, mode_state] = mode_stack.peek();
      this->active_mode.mode_index = mode_stack.size() - 1;
      this->active_mode.init_fn = resolve_hook(lisple_runtime, mode->init);
      this->active_mode.update_fn = resolve_hook(lisple_runtime, mode->update);
      this->active_mode.render_fn = resolve_hook(lisple_runtime, mode->render);
      this->active_mode.state = mode_state;

      this->hook_args.update_state(mode_state);

      /**
       * Re-build children for the mode that is now on top. State is not
       * persisted across a push/pop cycle - children re-initialize.
       */
      this->active_mode.children.clear();
      for (const auto& slot : mode->children)
      {
        this->active_mode.children.push_back(build_child_context(slot));
        init_child(this->active_mode.children.back());
      }
    }
  }

  void Session::push_mode(const Lisple::sptr_rtval& mode, const Lisple::sptr_rtval& state)
  {
    this->mode_stack.push(mode, state);

    auto& mode_obj = Lisple::obj<Mode>(*mode);

    /**
     * Resolve hooks in-place on first activation so that both active_mode
     * fields and any composition stack accesses find callables, not symbols.
     */
    mode_obj.init = resolve_hook(lisple_runtime, mode_obj.init);
    mode_obj.update = resolve_hook(lisple_runtime, mode_obj.update);
    mode_obj.render = resolve_hook(lisple_runtime, mode_obj.render);

    this->active_mode.mode_index = mode_stack.size() - 1;
    this->active_mode.init_fn = mode_obj.init;
    this->active_mode.update_fn = mode_obj.update;
    this->active_mode.render_fn = mode_obj.render;
    this->active_mode.state = state;

    if (!this->assets.is_loaded(mode_obj.name))
    {
      this->assets.load(mode_obj.name, mode_obj.resources);
    }

    this->hook_args.update_state(state);

    this->init_mode();

    this->active_mode.children.clear();
    for (const auto& slot : mode_obj.children)
    {
      this->active_mode.children.push_back(build_child_context(slot));
      init_child(this->active_mode.children.back());
    }
  }

  void Session::push_mode(const std::string& mode_name, const Lisple::sptr_rtval& state)
  {
    auto mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(mode_name));
    this->push_mode(mode, state);
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
        mode_stack.update_state(active_mode.state);
        push_mode(
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("mode"))->str(),
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("state")));
      }
      else if (type == "pop")
      {
        pop_mode();
      }
    }
  }

  void Session::init_mode()
  {
    auto new_state = invoke_hook(lisple_runtime,
                                 this->active_mode.init_fn,
                                 this->hook_args.init_args,
                                 this->active_mode.state);
    this->active_mode.state = new_state;
    this->hook_args.update_state(new_state);
  }

  void Session::update_mode()
  {
    auto update_stack = mode_stack.get_update_stack();

    for (size_t i = update_stack.size() - 1; i > 0; i--)
    {
      auto [mode, mode_state] = update_stack[i];

      Lisple::sptr_rtval_v rargs = this->hook_args.update_args;
      rargs[0] = mode_state;

      Lisple::sptr_rtval new_state =
        invoke_hook(lisple_runtime, mode->update, rargs, mode_state);
      mode_stack.update_state(new_state, update_stack.size() - i);
    }

    Lisple::sptr_rtval updated_state = invoke_hook(lisple_runtime,
                                                   this->active_mode.update_fn,
                                                   this->hook_args.update_args,
                                                   this->active_mode.state);
    this->hook_args.update_state(updated_state);
    this->active_mode.state = updated_state;

    for (auto& child : this->active_mode.children)
    {
      update_child(child);
    }
  }

  void Session::render_mode()
  {
    auto render_stack = mode_stack.get_render_stack();

    for (size_t i = render_stack.size() - 1; i > 0; i--)
    {
      auto [mode, mode_state] = render_stack[i];

      Lisple::sptr_rtval_v rargs = this->hook_args.render_args;
      rargs[0] = mode_state;

      invoke_hook(lisple_runtime, mode->render, rargs);
    }

    invoke_hook(lisple_runtime, this->active_mode.render_fn, this->hook_args.render_args);

    if (!this->active_mode.children.empty())
    {
      auto [top_mode, _] = mode_stack.peek();
      Rect parent_bounds = {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h};
      auto child_rects = layout_children(top_mode->children, parent_bounds);
      for (size_t i = 0; i < this->active_mode.children.size(); i++)
      {
        render_child(this->active_mode.children[i], child_rects[i]);
      }
    }
  }

  ChildContext Session::build_child_context(const ChildSlot& slot)
  {
    auto mode_val =
      Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(slot.mode_name));
    Mode* child_mode = &Lisple::obj<Mode>(*mode_val);

    child_mode->init = resolve_hook(lisple_runtime, child_mode->init);
    child_mode->update = resolve_hook(lisple_runtime, child_mode->update);
    child_mode->render = resolve_hook(lisple_runtime, child_mode->render);

    ChildContext ctx;
    ctx.mode = child_mode;
    ctx.state = Lisple::Constant::NIL;

    for (const auto& grandchild_slot : child_mode->children)
    {
      ctx.children.push_back(build_child_context(grandchild_slot));
    }

    return ctx;
  }

  void Session::init_child(ChildContext& child)
  {
    if (!this->assets.is_loaded(child.mode->name))
    {
      this->assets.load(child.mode->name, child.mode->resources);
    }

    {
      Lisple::sptr_rtval_v iargs = {child.state, this->hook_args.init_args[1]};
      auto new_state = invoke_hook(lisple_runtime, child.mode->init, iargs);
      if (new_state->type != Lisple::RTValue::Type::NIL) child.state = new_state;
    }

    for (auto& grandchild : child.children)
    {
      init_child(grandchild);
    }
  }

  void Session::update_child(ChildContext& child)
  {
    {
      Lisple::sptr_rtval_v uargs = {child.state,
                                    this->hook_args.update_args[1],
                                    this->hook_args.update_args[2]};
      child.state = invoke_hook(lisple_runtime, child.mode->update, uargs, child.state);
    }

    for (auto& grandchild : child.children)
    {
      update_child(grandchild);
    }
  }

  void Session::render_child(const ChildContext& child, const Rect& bounds)
  {
    SDL_Rect viewport = {bounds.x, bounds.y, bounds.w, bounds.h};
    SDL_RenderSetViewport(render_ctx.renderer, &viewport);

    Lisple::sptr_rtval_v rargs = {child.state, this->hook_args.render_args[1]};
    invoke_hook(lisple_runtime, child.mode->render, rargs);

    if (!child.children.empty())
    {
      /**
       * Child bounds here are local (origin at 0,0 within the viewport), but
       * SDL_RenderSetViewport expects absolute coordinates on the render target.
       * Offset by the parent's absolute position when recursing.
       */
      Rect local_parent = {0, 0, bounds.w, bounds.h};
      auto grandchild_rects = layout_children(child.mode->children, local_parent);
      for (size_t i = 0; i < child.children.size(); i++)
      {
        Rect abs = {bounds.x + grandchild_rects[i].x,
                    bounds.y + grandchild_rects[i].y,
                    grandchild_rects[i].w,
                    grandchild_rects[i].h};
        render_child(child.children[i], abs);
      }
    }

    SDL_RenderSetViewport(render_ctx.renderer, nullptr);
  }

  std::vector<Rect> Session::layout_children(const std::vector<ChildSlot>& slots,
                                             const Rect& parent)
  {
    int total_fixed = 0;
    int fill_count = 0;

    for (const auto& slot : slots)
    {
      const auto& constraint = slot.height;
      if (constraint.has_value() && constraint->kind == DimensionConstraint::Kind::FIXED)
        total_fixed += constraint->value;
      else
        fill_count++;
    }

    int fill_height = fill_count > 0 ? (parent.h - total_fixed) / fill_count : 0;

    std::vector<Rect> rects;
    int y = parent.y;
    for (const auto& slot : slots)
    {
      const auto& constraint = slot.height;
      int h =
        (constraint.has_value() && constraint->kind == DimensionConstraint::Kind::FIXED)
          ? constraint->value
          : fill_height;
      rects.push_back({parent.x, y, parent.w, h});
      y += h;
    }

    return rects;
  }

} // namespace Pixils::Runtime
