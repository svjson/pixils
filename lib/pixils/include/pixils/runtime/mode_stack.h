#ifndef PIXILS__RUNTIME__MODE_STACK_H
#define PIXILS__RUNTIME__MODE_STACK_H

#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  struct Mode;

  class ModeStack
  {
    Lisple::sptr_val stack;
    Lisple::sptr_val message_queue;

   public:
    ModeStack(const Lisple::sptr_val& stack, const Lisple::sptr_val& message_queue);

    void push(const Lisple::sptr_val& mode, const Lisple::sptr_val& state);
    std::pair<Mode*, Lisple::sptr_val> peek();
    void pop();

    void update_state(const Lisple::sptr_val& state, size_t offset = 0);

    size_t size() const;

    std::vector<std::pair<Mode*, Lisple::sptr_val>> get_render_stack();
    std::vector<std::pair<Mode*, Lisple::sptr_val>> get_update_stack();

    Lisple::sptr_val_v drain_messages();
  };
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_STACK_H */
