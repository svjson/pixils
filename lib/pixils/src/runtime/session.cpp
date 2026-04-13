
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

  /**
   * Extract a child's state slice from the parent's state map at the given
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

      /**
       * Restore active_mode from the Lisple stack. The saved state already
       * contains the full child state tree written back during the last update
       * cycle before the popped mode was pushed.
       */
      auto [mode, mode_state] = mode_stack.peek();
      active_mode.mode_index = mode_stack.size() - 1;
      active_mode.init_fn = mode->init;
      active_mode.update_fn = mode->update;
      active_mode.render_fn = mode->render;
      active_mode.state = mode_state;
      this->hook_args.update_state(mode_state);

      active_mode.children.clear();
      for (const auto& slot : mode->children)
      {
        active_mode.children.push_back(build_child_context(slot));
        restore_child_state(active_mode.children.back(), mode_state);
      }
    }
  }

  void Session::push_mode(const Lisple::sptr_rtval& mode, const Lisple::sptr_rtval& state)
  {
    /**
     * Flush the current active mode's state to the Lisple stack before pushing,
     * so pop_mode can recover it via peek() regardless of call site.
     */
    if (mode_stack.size() > 0) mode_stack.update_state(active_mode.state);

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

    /**
     * Build children and initialize each child, threading child states into
     * the parent state map as they complete.
     */
    this->active_mode.children.clear();
    auto parent_state = this->active_mode.state;
    for (const auto& slot : mode_obj.children)
    {
      this->active_mode.children.push_back(build_child_context(slot));
      parent_state = init_child(this->active_mode.children.back(), parent_state);
    }
    this->active_mode.state = parent_state;
    this->hook_args.update_state(parent_state);
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
    this->active_mode.state = updated_state;

    /**
     * Thread child updates through the parent state map. Each child reads its
     * slice from the parent, updates it, and writes the result back.
     */
    auto parent_state = this->active_mode.state;
    for (auto& child : this->active_mode.children)
      parent_state = update_child(child, parent_state);
    this->active_mode.state = parent_state;
    this->hook_args.update_state(parent_state);
  }

  void Session::render_full_mode(const ActiveMode& am, const Mode& mode_def)
  {
    Lisple::sptr_rtval_v rargs = this->hook_args.render_args;
    rargs[0] = am.state;
    invoke_hook(lisple_runtime, am.render_fn, rargs);

    if (!am.children.empty())
    {
      Rect parent_bounds = {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h};
      auto child_rects =
        layout_children(mode_def.children, parent_bounds, mode_def.layout_direction);
      for (size_t i = 0; i < am.children.size(); i++)
      {
        render_child(am.children[i], child_rects[i]);
      }
    }
  }

  void Session::render_mode()
  {
    auto render_stack = mode_stack.get_render_stack();

    /**
     * render_stack is top-first: [0]=top, [1]=just below, ..., [n-1]=bottom.
     * Render from bottom up, skipping index 0 (top mode is rendered last).
     */
    for (size_t i = render_stack.size() - 1; i > 0; i--)
    {
      auto [mode, state] = render_stack[i];
      render_mode_tree(*mode, state);
    }

    auto [top_mode, _] = mode_stack.peek();
    render_full_mode(active_mode, *top_mode);
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
    ctx.id = slot.id;
    ctx.mode = child_mode;
    ctx.state = Lisple::Constant::NIL;
    ctx.initial_state = slot.initial_state;

    for (const auto& grandchild_slot : child_mode->children)
    {
      ctx.children.push_back(build_child_context(grandchild_slot));
    }

    return ctx;
  }

  Lisple::sptr_rtval Session::init_child(ChildContext& child,
                                         const Lisple::sptr_rtval& parent_state)
  {
    if (!this->assets.is_loaded(child.mode->name))
    {
      this->assets.load(child.mode->name, child.mode->resources);
    }

    child.state = extract_child_state(parent_state, child.id);
    if (child.state->type == Lisple::RTValue::Type::NIL) child.state = child.initial_state;

    Lisple::sptr_rtval_v iargs = {child.state, this->hook_args.init_args[1]};
    auto new_state = invoke_hook(lisple_runtime, child.mode->init, iargs);
    if (new_state->type != Lisple::RTValue::Type::NIL) child.state = new_state;

    for (auto& grandchild : child.children)
      child.state = init_child(grandchild, child.state);

    return merge_child_state(parent_state, child.id, child.state);
  }

  Lisple::sptr_rtval Session::update_child(ChildContext& child,
                                           const Lisple::sptr_rtval& parent_state)
  {
    child.state = extract_child_state(parent_state, child.id);

    Lisple::sptr_rtval_v uargs = {child.state, this->hook_args.update_args[1]};
    child.state = invoke_hook(lisple_runtime, child.mode->update, uargs, child.state);

    for (auto& grandchild : child.children)
      child.state = update_child(grandchild, child.state);

    return merge_child_state(parent_state, child.id, child.state);
  }

  void Session::restore_child_state(ChildContext& child,
                                    const Lisple::sptr_rtval& parent_state)
  {
    child.state = extract_child_state(parent_state, child.id);
    for (auto& grandchild : child.children)
      restore_child_state(grandchild, child.state);
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
      auto grandchild_rects =
        layout_children(child.mode->children, local_parent, child.mode->layout_direction);
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

  void Session::render_mode_tree(const Mode& mode_def, const Lisple::sptr_rtval& state)
  {
    Lisple::sptr_rtval_v rargs = this->hook_args.render_args;
    rargs[0] = state;
    invoke_hook(lisple_runtime, mode_def.render, rargs);

    if (!mode_def.children.empty())
    {
      Rect parent_bounds = {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h};
      auto child_rects =
        layout_children(mode_def.children, parent_bounds, mode_def.layout_direction);
      for (size_t i = 0; i < mode_def.children.size(); i++)
      {
        const ChildSlot& slot = mode_def.children[i];
        auto child_state = extract_child_state(state, slot.id);
        auto child_mode_val =
          Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(slot.mode_name));
        render_child_tree(Lisple::obj<Mode>(*child_mode_val), child_state, child_rects[i]);
      }
    }
  }

  void Session::render_child_tree(const Mode& mode_def,
                                  const Lisple::sptr_rtval& state,
                                  const Rect& bounds)
  {
    SDL_Rect viewport = {bounds.x, bounds.y, bounds.w, bounds.h};
    SDL_RenderSetViewport(render_ctx.renderer, &viewport);

    Lisple::sptr_rtval_v rargs = {state, this->hook_args.render_args[1]};
    invoke_hook(lisple_runtime, mode_def.render, rargs);

    if (!mode_def.children.empty())
    {
      Rect local_parent = {0, 0, bounds.w, bounds.h};
      auto grandchild_rects =
        layout_children(mode_def.children, local_parent, mode_def.layout_direction);
      for (size_t i = 0; i < mode_def.children.size(); i++)
      {
        const ChildSlot& slot = mode_def.children[i];
        auto child_state = extract_child_state(state, slot.id);
        auto child_mode_val =
          Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(slot.mode_name));
        Rect abs = {bounds.x + grandchild_rects[i].x,
                    bounds.y + grandchild_rects[i].y,
                    grandchild_rects[i].w,
                    grandchild_rects[i].h};
        render_child_tree(Lisple::obj<Mode>(*child_mode_val), child_state, abs);
      }
    }

    SDL_RenderSetViewport(render_ctx.renderer, nullptr);
  }

  std::vector<Rect> Session::layout_children(const std::vector<ChildSlot>& slots,
                                             const Rect& parent,
                                             LayoutDirection direction)
  {
    bool row = direction == LayoutDirection::ROW;

    int total_fixed = 0;
    int fill_count = 0;

    for (const auto& slot : slots)
    {
      const auto& constraint = row ? slot.width : slot.height;
      if (constraint.has_value() && constraint->kind == DimensionConstraint::Kind::FIXED)
        total_fixed += constraint->value;
      else
        fill_count++;
    }

    int available = row ? parent.w : parent.h;
    int fill_size = fill_count > 0 ? (available - total_fixed) / fill_count : 0;

    std::vector<Rect> rects;
    int pos = row ? parent.x : parent.y;
    for (const auto& slot : slots)
    {
      const auto& constraint = row ? slot.width : slot.height;
      int size =
        (constraint.has_value() && constraint->kind == DimensionConstraint::Kind::FIXED)
          ? constraint->value
          : fill_size;
      if (row)
        rects.push_back({pos, parent.y, size, parent.h});
      else
        rects.push_back({parent.x, pos, parent.w, size});
      pos += size;
    }

    return rects;
  }

} // namespace Pixils::Runtime
