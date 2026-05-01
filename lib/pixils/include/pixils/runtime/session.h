
#ifndef __PIXILS__RUNTIME__SESSION_H_
#define __PIXILS__RUNTIME__SESSION_H_

#include <pixils/geom.h>
#include <pixils/runtime/hook_arguments.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/mode_stack.h>
#include <pixils/ui/mouse_state.h>

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
  struct View;

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
    UI::MouseState mouse_state;

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
    void update_mode();
    void render_mode();
  };

} // namespace Pixils::Runtime

#endif /* __PIXILS__RUNTIME__SESSION_H_ */
