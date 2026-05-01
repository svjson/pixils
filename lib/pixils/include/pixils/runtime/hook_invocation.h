#ifndef PIXILS__RUNTIME__HOOK_INVOCATION_H
#define PIXILS__RUNTIME__HOOK_INVOCATION_H

#include <lisple/form.h>
#include <memory>

namespace Lisple
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct View;

  Lisple::sptr_rtval invoke_hook(Lisple::Runtime& runtime,
                                 const std::shared_ptr<View>& view,
                                 const Lisple::sptr_rtval& fn,
                                 Lisple::sptr_rtval_v& args,
                                 const Lisple::sptr_rtval& fallback = Lisple::Constant::NIL);
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__HOOK_INVOCATION_H */
