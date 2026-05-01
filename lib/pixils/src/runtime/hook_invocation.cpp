#include "pixils/runtime/hook_invocation.h"

#include "pixils/hook_context.h"

#include <lisple/context.h>
#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  Lisple::sptr_rtval invoke_hook(Lisple::Runtime& runtime,
                                 const std::shared_ptr<View>& view,
                                 const Lisple::sptr_rtval& fn,
                                 Lisple::sptr_rtval_v& args,
                                 const Lisple::sptr_rtval& fallback)
  {
    if (!fn || fn->type == Lisple::RTValue::Type::NIL) return fallback;

    Lisple::obj<HookContext>(*args.back()).current_view = view;
    Lisple::Context exec_ctx(runtime);
    return fn->exec().execute(exec_ctx, args);
  }
} // namespace Pixils::Runtime
