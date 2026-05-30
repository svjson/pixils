
#include "pixils/runtime/mode_stack.h"

#include <pixils/runtime/mode.h>

#include <roo/host/object.h>
#include <roo/runtime/seq.h>

namespace Pixils::Runtime
{
  ModeStack::ModeStack(const Roo::sptr_val& stack, const Roo::sptr_val& message_queue)
    : stack(stack)
    , message_queue(message_queue)
  {
  }

  void ModeStack::push(const Roo::sptr_val& mode, const Roo::sptr_val& state)
  {
    Roo::append(*stack, Roo::vector({mode, state}));
  }

  std::pair<Mode*, Roo::sptr_val> ModeStack::peek()
  {
    auto frame = Roo::get_child(*stack, size() - 1);

    return std::make_pair(&Roo::obj<Mode>(*Roo::get_child(*frame, 0)),
                          Roo::get_child(*frame, 1));
  }

  void ModeStack::pop()
  {
    Roo::pop_child(*stack);
  }

  void ModeStack::update_state(const Roo::sptr_val& state, size_t offset)
  {
    auto frame = Roo::get_child(*stack, size() - 1 - offset);

    if (frame->type != Roo::Value::Type::NIL)
    {
      std::get<Roo::sptr_val_v>(frame->value).at(1) = state;
    }
  }

  std::vector<std::pair<Mode*, Roo::sptr_val>> ModeStack::get_update_stack()
  {
    std::vector<std::pair<Mode*, Roo::sptr_val>> update_stack;

    for (int i = this->size() - 1; i >= 0; i--)
    {
      auto frame = Roo::get_child(*stack, i);
      Mode* mode = &Roo::obj<Mode>(*Roo::get_child(*frame, 0));
      update_stack.push_back(std::make_pair(mode, Roo::get_child(*frame, 1)));
      if (!mode->composition.update)
      {
        break;
      }
    }

    return update_stack;
  }

  std::vector<std::pair<Mode*, Roo::sptr_val>> ModeStack::get_render_stack()
  {
    std::vector<std::pair<Mode*, Roo::sptr_val>> render_stack;

    for (int i = this->size() - 1; i >= 0; i--)
    {
      auto frame = Roo::get_child(*stack, i);
      Mode* mode = &Roo::obj<Mode>(*Roo::get_child(*frame, 0));
      render_stack.push_back(std::make_pair(mode, Roo::get_child(*frame, 1)));
      if (!mode->composition.render)
      {
        break;
      }
    }

    return render_stack;
  }

  size_t ModeStack::size() const
  {
    return Roo::count(*stack);
  }

  Roo::sptr_val_v ModeStack::drain_messages()
  {
    Roo::sptr_val_v& persistent_vector =
      std::get<Roo::sptr_val_v>(message_queue->value);

    Roo::sptr_val_v messages = persistent_vector;

    persistent_vector.clear();

    return messages;
  }

} // namespace Pixils::Runtime
