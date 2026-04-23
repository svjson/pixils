
#ifndef __PIXILS__RUNTIME__SESSION_H_
#define __PIXILS__RUNTIME__SESSION_H_

#include <pixils/frame_events.h>
#include <pixils/geom.h>
#include <pixils/runtime/event_routing.h>
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
    FrameEvents* events = nullptr;

    Lisple::sptr_rtval_v init_args = {Lisple::Constant::NIL, ctx};
    Lisple::sptr_rtval_v update_args = {Lisple::Constant::NIL, ctx};
    Lisple::sptr_rtval_v render_args = {Lisple::Constant::NIL, ctx};

    void update_state(const Lisple::sptr_rtval& state);
  };

  struct Session
  {
    Lisple::Runtime& lisple_runtime;
    Asset::Registry& assets;
    RenderContext& render_ctx;
    ModeStack mode_stack;
    Lisple::sptr_rtval modes;
    std::shared_ptr<View> active_mode;
    std::vector<std::shared_ptr<View>> ctx_stack;
    HookArguments hook_args;
    EventRouter event_router;

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

    std::shared_ptr<View> build_view(const ChildSlot& slot);
    Lisple::sptr_rtval init_view(View& view, const Lisple::sptr_rtval& parent_state);
    void restore_view_state(View& view, const Lisple::sptr_rtval& parent_state);

   private:
    void init_mode();
  };

} // namespace Pixils::Runtime

#endif /* __PIXILS__RUNTIME__SESSION_H_ */
