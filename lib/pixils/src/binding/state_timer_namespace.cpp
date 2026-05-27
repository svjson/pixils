#include <pixils/binding/state_timer_namespace.h>

#include <algorithm>
#include <lisple/context.h>
#include <lisple/exception.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  namespace TimerMapKey
  {
    inline const std::string INTERVAL_MS = "interval-ms";
    inline const std::string LAST_TICK_MS = "last-tick-ms";
    inline const std::string STARTED = "started?";
    inline const std::string TICKED = "ticked?";
  } // namespace TimerMapKey

  NATIVE_ADAPTER_IMPL(TimerAdapter,
                      State::Timer,
                      &HostType::TIMER,
                      (TimerMapKey::INTERVAL_MS, interval_ms),
                      (TimerMapKey::LAST_TICK_MS, last_tick_ms),
                      (TimerMapKey::STARTED, started),
                      (TimerMapKey::TICKED, ticked));

  NOBJ_PROP_GET__FIELD(TimerAdapter, interval_ms);
  NOBJ_PROP_GET__FIELD(TimerAdapter, last_tick_ms);
  NOBJ_PROP_GET__FIELD(TimerAdapter, started);
  NOBJ_PROP_GET__FIELD(TimerAdapter, ticked);

  namespace Function
  {
    FUNC_IMPL(MakeTimer,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeTimer::exec_make))));

    Lisple::MapSchema timer_schema({},
                                   {{TimerMapKey::INTERVAL_MS, &Lisple::Type::NUMBER},
                                    {TimerMapKey::LAST_TICK_MS, &Lisple::Type::NUMBER},
                                    {TimerMapKey::STARTED, &Lisple::Type::BOOL},
                                    {TimerMapKey::TICKED, &Lisple::Type::BOOL}});

    EXEC_BODY(MakeTimer, exec_make)
    {
      auto opts = timer_schema.bind(ctx, *args[0]);

      Pixils::State::Timer timer{};
      timer.interval_ms =
        std::max(0L, opts.optional<long>(TimerMapKey::INTERVAL_MS).value_or(1000));
      timer.last_tick_ms = opts.optional<long>(TimerMapKey::LAST_TICK_MS).value_or(0);
      timer.started = opts.boolean(TimerMapKey::STARTED, false);
      timer.ticked = opts.boolean(TimerMapKey::TICKED, false);

      return TimerAdapter::make_unique(timer);
    }

    FUNC_IMPL(TickTimer,
              SIG((FN_ARGS((&HostType::TIMER)), EXEC_DISPATCH(&TickTimer::exec_tick))));

    EXEC_BODY(TickTimer, exec_tick)
    {
      return TimerAdapter::make_unique(
        Pixils::State::tick_timer(Lisple::obj<Pixils::State::Timer>(*args[0])));
    }

    FUNC_IMPL(TickTimerAt,
              SIG((FN_ARGS((&Lisple::Type::MAP), (&Lisple::Type::KEYWORD)),
                   EXEC_DISPATCH(&TickTimerAt::exec_tick_at))));

    EXEC_BODY(TickTimerAt, exec_tick_at)
    {
      auto current_timer = Lisple::Dict::get_property(args[0], args[1]);
      auto ticked_timer =
        Pixils::State::tick_timer(Lisple::obj<Pixils::State::Timer>(*current_timer));

      Lisple::sptr_val_v path = {args[1]};
      return Lisple::Dict::assoc_in(args[0], path, TimerAdapter::make_unique(ticked_timer));
    }

    FUNC_IMPL(TimerTicked,
              SIG((FN_ARGS((&HostType::TIMER)), EXEC_DISPATCH(&TimerTicked::exec_ticked))));

    EXEC_BODY(TimerTicked, exec_ticked)
    {
      return Lisple::Value::boolean(Lisple::obj<Pixils::State::Timer>(*args[0]).ticked);
    }

    FUNC_IMPL(TimerTickedAt,
              SIG((FN_ARGS((&Lisple::Type::MAP), (&Lisple::Type::KEYWORD)),
                   EXEC_DISPATCH(&TimerTickedAt::exec_ticked_at))));

    EXEC_BODY(TimerTickedAt, exec_ticked_at)
    {
      auto timer = Lisple::Dict::get_property(args[0], args[1]);
      return Lisple::Value::boolean(Lisple::obj<Pixils::State::Timer>(*timer).ticked);
    }
  } // namespace Function

  StateTimerNamespace::StateTimerNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__STATE__TIMER))
  {
    values.emplace(FN__TIMER_MAKE, Function::MakeTimer::make());
    values.emplace(FN__TIMER_TICK, Function::TickTimer::make());
    values.emplace(FN__TIMER_TICK_AT, Function::TickTimerAt::make());
    values.emplace(FN__TIMER_TICKED, Function::TimerTicked::make());
    values.emplace(FN__TIMER_TICKED_AT, Function::TimerTickedAt::make());
  }
} // namespace Pixils::Script
