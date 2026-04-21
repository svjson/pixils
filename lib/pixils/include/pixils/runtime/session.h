
#ifndef __PIXILS__RUNTIME__SESSION_H_
#define __PIXILS__RUNTIME__SESSION_H_

#include <pixils/frame_events.h>
#include <pixils/geom.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/mode_stack.h>
#include <pixils/ui/event.h>

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
    FrameEvents* events = nullptr;

    Lisple::sptr_rtval_v init_args = {Lisple::Constant::NIL, ctx};
    Lisple::sptr_rtval_v update_args = {Lisple::Constant::NIL, ctx};
    Lisple::sptr_rtval_v render_args = {Lisple::Constant::NIL, ctx};

    void update_state(const Lisple::sptr_rtval& state);
  };

  /**
   * Live instance of a mode. Serves as the runtime companion for any mode -
   * whether active at the top of the mode stack, participating in composition
   * below it, or placed as a layout child of another mode. Holds the resolved
   * mode pointer, Lisple state, last-computed layout bounds, and any nested
   * child views. For layout children, `id` is the key under which this
   * view's state is stored in the parent state map.
   */
  struct View
  {
    std::string id;
    Mode* mode = nullptr;
    /**
     * Owns a per-instance copy of the mode when push-time or
     * child-slot overrides are present. `mode` points here instead of
     * into the shared registry. unique_ptr so that the pointer
     * remains valid when View is moved.
     */
    std::unique_ptr<Mode> owned_mode;
    Lisple::sptr_rtval state = Lisple::Constant::NIL;
    Lisple::sptr_rtval initial_state = Lisple::Constant::NIL;
    Rect bounds = {0, 0, 0, 0};
    std::vector<View> children;
  };

  struct Session
  {
    Lisple::Runtime& lisple_runtime;
    Asset::Registry& assets;
    RenderContext& render_ctx;
    ModeStack mode_stack;
    Lisple::sptr_rtval modes;
    View active_mode;
    std::vector<View> ctx_stack;
    HookArguments hook_args;
    std::optional<MouseEvent> active_mouse_event;

    Session(Lisple::Runtime& lisple_runtime,
            Asset::Registry& assets,
            RenderContext& render_ctx,
            const HookArguments& hook_args);

    void pop_mode();
    void push_mode(const Lisple::sptr_rtval& mode,
                   const Lisple::sptr_rtval& state,
                   const Lisple::sptr_rtval& overrides = Lisple::Constant::NIL);
    void push_mode(const std::string& mode_name,
                   const Lisple::sptr_rtval& state,
                   const Lisple::sptr_rtval& overrides = Lisple::Constant::NIL);
    void process_messages();
    Lisple::sptr_rtval invoke_hook(
      const Lisple::sptr_rtval& fn,
      Lisple::sptr_rtval_v& args,
      const Lisple::sptr_rtval& fallback = Lisple::Constant::NIL);
    void update_mode();
    void render_mode();

    View build_view(const ChildSlot& slot);
    Lisple::sptr_rtval init_view(View& view, const Lisple::sptr_rtval& parent_state);
    Lisple::sptr_rtval update_view(View& view, const Lisple::sptr_rtval& parent_state);
    void restore_view_state(View& view, const Lisple::sptr_rtval& parent_state);

   private:
    void init_mode();
  };

} // namespace Pixils::Runtime

#endif /* __PIXILS__RUNTIME__SESSION_H_ */
