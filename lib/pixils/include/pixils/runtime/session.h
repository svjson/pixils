
#ifndef __PIXILS__RUNTIME__SESSION_H_
#define __PIXILS__RUNTIME__SESSION_H_

#include <pixils/geom.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/mode_stack.h>

#include <lisple/form.h>

namespace Pixils
{
  struct RenderContext;
}

namespace Pixils::Asset
{
  class Registry;
}

namespace Pixils::Runtime
{
  struct HookArguments
  {
    Lisple::sptr_rtval ctx;

    Lisple::sptr_rtval_v init_args = {Lisple::Constant::NIL, ctx};
    Lisple::sptr_rtval_v update_args = {Lisple::Constant::NIL, ctx};
    Lisple::sptr_rtval_v render_args = {Lisple::Constant::NIL, ctx};

    void update_state(const Lisple::sptr_rtval& state);
  };

  /**
   * Runtime state for a single child mode in a layout tree. Each node holds the
   * resolved mode pointer, the child's own Lisple state, and any nested children.
   * `id` matches the corresponding ChildSlot id and is used as the key in the
   * parent's state map where this child's state is stored.
   */
  struct ChildContext
  {
    std::string id;
    Mode* mode = nullptr;
    Lisple::sptr_rtval state = Lisple::Constant::NIL;
    Lisple::sptr_rtval initial_state = Lisple::Constant::NIL;
    std::vector<ChildContext> children;
  };

  struct ActiveMode
  {
    int mode_index = -1;

    Lisple::sptr_rtval init_fn = Lisple::Constant::NIL;
    Lisple::sptr_rtval update_fn = Lisple::Constant::NIL;
    Lisple::sptr_rtval render_fn = Lisple::Constant::NIL;

    Lisple::sptr_rtval state = Lisple::Constant::NIL;
    std::vector<ChildContext> children;
  };

  struct Session
  {
    Lisple::Runtime& lisple_runtime;
    Asset::Registry& assets;
    RenderContext& render_ctx;
    ModeStack mode_stack;
    Lisple::sptr_rtval modes;
    ActiveMode active_mode;
    HookArguments hook_args;

    Session(Lisple::Runtime& lisple_runtime,
            Asset::Registry& assets,
            RenderContext& render_ctx,
            const HookArguments& hook_args);

    void pop_mode();
    void push_mode(const Lisple::sptr_rtval& mode, const Lisple::sptr_rtval& state);
    void push_mode(const std::string& mode_name, const Lisple::sptr_rtval& state);
    void process_messages();
    void init_mode();
    void update_mode();
    void render_mode();

    ChildContext build_child_context(const ChildSlot& slot);
    Lisple::sptr_rtval init_child(ChildContext& child, const Lisple::sptr_rtval& parent_state);
    Lisple::sptr_rtval update_child(ChildContext& child, const Lisple::sptr_rtval& parent_state);
    void restore_child_state(ChildContext& child, const Lisple::sptr_rtval& parent_state);
    void render_child(const ChildContext& child, const Rect& bounds);
    void render_full_mode(const ActiveMode& am, const Mode& mode_def);
    void render_mode_tree(const Mode& mode_def, const Lisple::sptr_rtval& state);
    void render_child_tree(const Mode& mode_def,
                           const Lisple::sptr_rtval& state,
                           const Rect& bounds);

    std::vector<Rect> layout_children(const std::vector<ChildSlot>& slots,
                                      const Rect& parent);
  };

} // namespace Pixils::Runtime

#endif /* __PIXILS__RUNTIME__SESSION_H_ */
