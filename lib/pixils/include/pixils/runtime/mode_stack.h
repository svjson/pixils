#ifndef PIXILS__RUNTIME__MODE_STACK_H
#define PIXILS__RUNTIME__MODE_STACK_H

#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  struct Mode;

  class ModeStack
  {
    Lisple::sptr_rtval stack;
    Lisple::sptr_rtval message_queue;

   public:
    ModeStack(const Lisple::sptr_rtval& stack, const Lisple::sptr_rtval& message_queue);

    void push(const Lisple::sptr_rtval& mode, const Lisple::sptr_rtval& state);
    std::pair<Mode*, Lisple::sptr_rtval> peek();
    void pop();

    void update_state(const Lisple::sptr_rtval& state);

    size_t size() const;

    Lisple::sptr_rtval_v drain_messages();
  };
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_STACK_H */
