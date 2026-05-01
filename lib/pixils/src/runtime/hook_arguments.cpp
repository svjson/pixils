#include "pixils/runtime/hook_arguments.h"

namespace Pixils::Runtime
{
  void HookArguments::update_state(const Lisple::sptr_rtval& state)
  {
    this->init_args[0] = state;
    this->update_args[0] = state;
    this->render_args[0] = state;
  }
} // namespace Pixils::Runtime
