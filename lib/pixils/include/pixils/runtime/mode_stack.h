#ifndef PIXILS__RUNTIME__MODE_STACK_H
#define PIXILS__RUNTIME__MODE_STACK_H

#include <roo/runtime/value.h>

namespace Pixils::Runtime
{
  struct Mode;

  class ModeStack
  {
    Roo::sptr_val stack;
    Roo::sptr_val message_queue;

   public:
    ModeStack(const Roo::sptr_val& stack, const Roo::sptr_val& message_queue);

    void push(const Roo::sptr_val& mode, const Roo::sptr_val& state);
    std::pair<Mode*, Roo::sptr_val> peek();
    void pop();

    void update_state(const Roo::sptr_val& state, size_t offset = 0);

    size_t size() const;

    std::vector<std::pair<Mode*, Roo::sptr_val>> get_render_stack();
    std::vector<std::pair<Mode*, Roo::sptr_val>> get_update_stack();

    Roo::sptr_val_v drain_messages();
  };
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_STACK_H */
