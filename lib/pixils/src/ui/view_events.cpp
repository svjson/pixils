#include "pixils/ui/view_events.h"

#include <pixils/benchmark/counters.h>
#include <pixils/binding/ui/ui_namespace.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>

#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>

namespace Pixils::UI
{
  namespace
  {
    void restore_subtree_state(const std::shared_ptr<Runtime::View>& view,
                               const Lisple::sptr_val& parent_state)
    {
      view->state = Runtime::extract_state(parent_state, *view);
      for (auto& child : view->children)
      {
        restore_subtree_state(child, view->state);
      }
    }

  } // namespace

  std::vector<CustomEvent> process_view_events(Runtime::View& receiver,
                                               Lisple::sptr_val* parent_state,
                                               Lisple::sptr_val& view_ctx,
                                               std::vector<CustomEvent>& events,
                                               Lisple::Runtime& runtime,
                                               bool* receiver_state_updated)
  {
    std::vector<CustomEvent> bubbled_events;
    for (auto& event : events)
    {
      auto it = receiver.mode->event_handlers.find(event.event_key->str());
      if (it == receiver.mode->event_handlers.end() ||
          it->second->type != Lisple::Value::Type::FUNCTION)
      {
        PIXILS_BENCHMARK_COUNT(events_bubbled);
        bubbled_events.push_back(event);
        continue;
      }

      PIXILS_BENCHMARK_COUNT(event_handler_invocations);
      auto event_ref = Script::CustomEventAdapter::make_ref(event);
      Lisple::sptr_val_v event_args{receiver.state, event_ref, view_ctx};
      auto receiver_ref = std::shared_ptr<Runtime::View>(&receiver, [](Runtime::View*) {});
      auto new_state =
        Runtime::invoke_hook(runtime, receiver_ref, it->second, event_args, receiver.state);
      if (new_state->type != Lisple::Value::Type::NIL)
      {
        receiver.state = new_state;
        if (receiver_state_updated) *receiver_state_updated = true;
        for (auto& child : receiver.children)
        {
          restore_subtree_state(child, receiver.state);
        }
        if (parent_state)
        {
          *parent_state = Runtime::merge_state(*parent_state, receiver, receiver.state);
        }
      }
      if (!event.propagation_stopped)
      {
        PIXILS_BENCHMARK_COUNT(events_bubbled);
        bubbled_events.push_back(event);
      }
    }
    return bubbled_events;
  }

} // namespace Pixils::UI
