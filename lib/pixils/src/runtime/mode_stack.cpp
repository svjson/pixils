
#include "pixils/runtime/mode_stack.h"

#include <pixils/runtime/mode.h>

#include <lisple/host/object.h>
#include <lisple/runtime/seq.h>

namespace Pixils::Runtime
{
  ModeStack::ModeStack(const Lisple::sptr_val& stack, const Lisple::sptr_val& message_queue)
    : stack(stack)
    , message_queue(message_queue)
  {
  }

  void ModeStack::push(const Lisple::sptr_val& mode, const Lisple::sptr_val& state)
  {
    Lisple::append(*stack, Lisple::vector({mode, state}));
  }

  std::pair<Mode*, Lisple::sptr_val> ModeStack::peek()
  {
    auto frame = Lisple::get_child(*stack, size() - 1);

    return std::make_pair(&Lisple::obj<Mode>(*Lisple::get_child(*frame, 0)),
                          Lisple::get_child(*frame, 1));
  }

  void ModeStack::pop()
  {
    Lisple::pop_child(*stack);
  }

  void ModeStack::update_state(const Lisple::sptr_val& state, size_t offset)
  {
    auto frame = Lisple::get_child(*stack, size() - 1 - offset);

    if (frame->type != Lisple::Value::Type::NIL)
    {
      std::get<Lisple::sptr_val_v>(frame->value).at(1) = state;
    }
  }

  std::vector<std::pair<Mode*, Lisple::sptr_val>> ModeStack::get_update_stack()
  {
    std::vector<std::pair<Mode*, Lisple::sptr_val>> update_stack;

    for (int i = this->size() - 1; i >= 0; i--)
    {
      auto frame = Lisple::get_child(*stack, i);
      Mode* mode = &Lisple::obj<Mode>(*Lisple::get_child(*frame, 0));
      update_stack.push_back(std::make_pair(mode, Lisple::get_child(*frame, 1)));
      if (!mode->composition.update)
      {
        break;
      }
    }

    return update_stack;
  }

  std::vector<std::pair<Mode*, Lisple::sptr_val>> ModeStack::get_render_stack()
  {
    std::vector<std::pair<Mode*, Lisple::sptr_val>> render_stack;

    for (int i = this->size() - 1; i >= 0; i--)
    {
      auto frame = Lisple::get_child(*stack, i);
      Mode* mode = &Lisple::obj<Mode>(*Lisple::get_child(*frame, 0));
      render_stack.push_back(std::make_pair(mode, Lisple::get_child(*frame, 1)));
      if (!mode->composition.render)
      {
        break;
      }
    }

    return render_stack;
  }

  size_t ModeStack::size() const
  {
    return Lisple::count(*stack);
  }

  Lisple::sptr_val_v ModeStack::drain_messages()
  {
    Lisple::sptr_val_v& persistent_vector =
      std::get<Lisple::sptr_val_v>(message_queue->value);

    Lisple::sptr_val_v messages = persistent_vector;

    persistent_vector.clear();

    return messages;
  }

} // namespace Pixils::Runtime
