
#include "pixils/runtime/mode_stack.h"

#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/runtime/seq.h>

namespace Pixils::Runtime
{
  ModeStack::ModeStack(const Lisple::sptr_rtval& stack,
                       const Lisple::sptr_rtval& message_queue)
    : stack(stack)
    , message_queue(message_queue)
  {
  }

  void ModeStack::push(const Lisple::sptr_rtval& mode, const Lisple::sptr_rtval& state)
  {
    Lisple::append(*stack, Lisple::RTValue::vector({mode, state}));
  }

  std::pair<Mode*, Lisple::sptr_rtval> ModeStack::peek()
  {
    auto frame = Lisple::get_child(*stack, size() - 1);

    return std::make_pair(&Lisple::obj<Mode>(*Lisple::get_child(*frame, 0)),
                          Lisple::get_child(*frame, 1));
  }

  void ModeStack::pop()
  {
    Lisple::pop_child(*stack);
  }

  void ModeStack::update_state(const Lisple::sptr_rtval& state)
  {
    auto frame = Lisple::get_child(*stack, size() - 1);

    if (frame->type != Lisple::RTValue::Type::NIL)
    {
      std::get<Lisple::sptr_rtval_v>(frame->value).at(1) = state;
    }
  }

  size_t ModeStack::size() const
  {
    return Lisple::count(*stack);
  }

  Lisple::sptr_rtval_v ModeStack::drain_messages()
  {
    Lisple::sptr_rtval_v& persistent_vector =
      std::get<Lisple::sptr_rtval_v>(message_queue->value);

    Lisple::sptr_rtval_v messages = persistent_vector;

    persistent_vector.clear();

    return messages;
  }

} // namespace Pixils::Runtime
