#ifndef PIXILS__STATE_COUNTER_NAMESPACE_H
#define PIXILS__STATE_COUNTER_NAMESPACE_H

#include <pixils/state/counter.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>
#include <string>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__STATE__COUNTER = "pixils.state.counter";

  inline constexpr std::string_view FN__MAKE_COUNTER = "make";
  inline constexpr std::string_view FN__ADVANCE = "advance";
  inline constexpr std::string_view FN__ADVANCE_AT = "advance-at";
  inline constexpr std::string_view FN__VALUE = "value";
  inline constexpr std::string_view FN__VALUE_AT = "value-at";
  inline const std::string FN__PIXILS__STATE__COUNTER__MAKE = "pixils.state.counter/make";

  namespace HostType
  {
    HOST_TYPE(COUNTER, "HCounter", FN__PIXILS__STATE__COUNTER__MAKE);
  }

  namespace Function
  {
    FUNC(MakeCounter, make);
    FUNC(AdvanceCounter, advance);
    FUNC(AdvanceCounterAt, advance_at, advance_at_with_triggers);
    FUNC(CounterValue, value);
    FUNC(CounterValueAt, value_at);
  } // namespace Function

  NATIVE_ADAPTER(CounterAdapter,
                 State::Counter,
                 (value, wrapped, start, end, every, phase, mode));

  class StateCounterNamespace : public Lisple::Namespace
  {
   public:
    StateCounterNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__STATE_COUNTER_NAMESPACE_H */
