#ifndef PIXILS__RUNTIME__HOOK_INVOCATION_H
#define PIXILS__RUNTIME__HOOK_INVOCATION_H

#include <roo/form.h>
#include <memory>

namespace Roo
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct View;

  Roo::sptr_val invoke_hook(Roo::Runtime& runtime,
                               const std::shared_ptr<View>& view,
                               const Roo::sptr_val& fn,
                               Roo::sptr_val_v& args,
                               const Roo::sptr_val& fallback = Roo::Constant::NIL);
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__HOOK_INVOCATION_H */
