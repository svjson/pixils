
#ifndef __PIXILS__RUNTIME__SESSION_H_
#define __PIXILS__RUNTIME__SESSION_H_

#include <pixils/runtime/mode_stack.h>

#include <lisple/form.h>

namespace Pixils::Asset
{
  class Registry;
}

namespace Pixils::Runtime
{
  struct HookArguments
  {
    Lisple::sptr_rtval events;
    Lisple::sptr_rtval ctx;

    Lisple::sptr_rtval_v init_args = {Lisple::Constant::NIL, ctx};
    Lisple::sptr_rtval_v update_args = {Lisple::Constant::NIL, events, ctx};
    Lisple::sptr_rtval_v render_args = {Lisple::Constant::NIL, ctx};

    void update_state(const Lisple::sptr_rtval& state);
  };

  struct ActiveMode
  {
    int mode_index = -1;

    std::string init_fun;
    std::string update_fun;
    std::string render_fun;

    Lisple::sptr_rtval state = Lisple::Constant::NIL;
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
    void update_state(const Lisple::sptr_rtval& state, size_t index);
    void process_messages();
    void init_mode();
    void update_mode();
    void render_mode();
  };

} // namespace Pixils::Runtime

#endif /* __PIXILS__RUNTIME__SESSION_H_ */
