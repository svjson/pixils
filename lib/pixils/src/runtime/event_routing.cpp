#include "pixils/runtime/event_routing.h"

#include "pixils/runtime/state.h"
#include "pixils/runtime/view.h"

#include <lisple/context.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    Lisple::sptr_rtval invoke(const Lisple::sptr_rtval& fn,
                              Lisple::sptr_rtval_v& args,
                              Lisple::Runtime& rt,
                              const Lisple::sptr_rtval& fallback = Lisple::Constant::NIL)
    {
      if (!fn || fn->type == Lisple::RTValue::Type::NIL) return fallback;
      Lisple::Context exec_ctx(rt);
      return fn->exec().execute(exec_ctx, args);
    }

    void restore_subtree_state(const std::shared_ptr<View>& view,
                               const Lisple::sptr_rtval& parent_state)
    {
      view->state = extract_state(parent_state, *view);
      for (auto& child : view->children)
      {
        restore_subtree_state(child, view->state);
      }
    }

  } // namespace

  std::vector<CustomEvent> process_view_events(View& receiver,
                                               Lisple::sptr_rtval* parent_state,
                                               Lisple::sptr_rtval& view_ctx,
                                               std::vector<CustomEvent>& events,
                                               Lisple::Runtime& runtime,
                                               bool* receiver_state_updated)
  {
    std::vector<CustomEvent> bubbled_events;
    for (auto& event : events)
    {
      auto it = receiver.mode->event_handlers.find(event.event_key->str());
      if (it == receiver.mode->event_handlers.end() ||
          it->second->type != Lisple::RTValue::Type::FUNCTION)
      {
        bubbled_events.push_back(event);
        continue;
      }

      Lisple::sptr_rtval_v event_args{receiver.state, event.payload, view_ctx};
      auto new_state = invoke(it->second, event_args, runtime, receiver.state);
      if (new_state->type != Lisple::RTValue::Type::NIL)
      {
        receiver.state = new_state;
        if (receiver_state_updated) *receiver_state_updated = true;
        for (auto& child : receiver.children)
        {
          restore_subtree_state(child, receiver.state);
        }
        if (parent_state)
        {
          *parent_state = merge_state(*parent_state, receiver, receiver.state);
        }
      }
      if (!event.propagation_stopped)
      {
        bubbled_events.push_back(event);
      }
    }
    return bubbled_events;
  }

} // namespace Pixils::Runtime
