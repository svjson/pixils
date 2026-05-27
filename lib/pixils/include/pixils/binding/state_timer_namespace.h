#ifndef PIXILS__STATE_TIMER_NAMESPACE_H
#define PIXILS__STATE_TIMER_NAMESPACE_H

#include <pixils/state/timer.h>

#include <lisple/exec.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>
#include <string>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__STATE__TIMER = "pixils.state.timer";

  inline constexpr std::string_view FN__TIMER_MAKE = "make";
  inline constexpr std::string_view FN__TIMER_TICK = "tick";
  inline constexpr std::string_view FN__TIMER_TICK_AT = "tick-at";
  inline constexpr std::string_view FN__TIMER_TICKED = "ticked?";
  inline constexpr std::string_view FN__TIMER_TICKED_AT = "ticked-at?";
  inline const std::string FN__PIXILS__STATE__TIMER__MAKE = "pixils.state.timer/make";

  namespace HostType
  {
    HOST_TYPE(TIMER, "HTimer", FN__PIXILS__STATE__TIMER__MAKE);
  }

  namespace Function
  {
    FUNC(MakeTimer, make);
    FUNC(TickTimer, tick);
    FUNC(TickTimerAt, tick_at);
    FUNC(TimerTicked, ticked);
    FUNC(TimerTickedAt, ticked_at);
  } // namespace Function

  NATIVE_ADAPTER(TimerAdapter, State::Timer, (interval_ms, last_tick_ms, started, ticked));

  class StateTimerNamespace : public Lisple::Namespace
  {
   public:
    StateTimerNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__STATE_TIMER_NAMESPACE_H */
