#ifndef PIXILS__RUNTIME__HOOK_ARGUMENTS_H
#define PIXILS__RUNTIME__HOOK_ARGUMENTS_H

#include <pixils/frame_events.h>

#include <lisple/form.h>

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
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__HOOK_ARGUMENTS_H */
