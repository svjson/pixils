#ifndef PIXILS__RUNTIME__HOOK_ARGUMENTS_H
#define PIXILS__RUNTIME__HOOK_ARGUMENTS_H

#include <pixils/frame_events.h>

#include <roo/form.h>

namespace Pixils::Runtime
{
  struct HookArguments
  {
    Roo::sptr_val ctx;
    FrameEvents* events = nullptr;

    Roo::sptr_val_v init_args = {Roo::Constant::NIL, ctx};
    Roo::sptr_val_v update_args = {Roo::Constant::NIL, ctx};
    Roo::sptr_val_v render_args = {Roo::Constant::NIL, ctx};

    void update_state(const Roo::sptr_val& state);
  };
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__HOOK_ARGUMENTS_H */
