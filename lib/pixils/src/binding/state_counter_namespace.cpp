#include <pixils/binding/state_counter_namespace.h>

#include <algorithm>
#include <lisple/context.h>
#include <lisple/exception.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Script
{
  namespace MapKey
  {
    inline const std::string START = "start";
    inline const std::string VALUE = "value";
    inline const std::string END = "end";
    inline const std::string EVERY = "every";
    inline const std::string MODE = "mode";
    inline const std::string PHASE = "phase";
    inline const std::string WRAPPED = "wrapped?";
    inline const std::string ON_STEP = "on-step";
    inline const std::string ON_WRAP = "on-wrap";
  } // namespace MapKey

  namespace
  {

    Pixils::State::CounterMode parse_mode(const Lisple::sptr_rtval& mode_val)
    {
      if (!mode_val || mode_val->type == Lisple::RTValue::Type::NIL)
      {
        return Pixils::State::CounterMode::LOOP;
      }

      const std::string mode = mode_val->str();
      if (mode == "stop")
      {
        return Pixils::State::CounterMode::STOP;
      }

      return Pixils::State::CounterMode::LOOP;
    }

    Lisple::sptr_rtval invoke_callback(Lisple::Context& ctx,
                                       const Lisple::sptr_rtval& callback,
                                       const Lisple::sptr_rtval& value)
    {
      if (!callback || callback->type == Lisple::RTValue::Type::NIL)
      {
        return value;
      }

      Lisple::sptr_rtval resolved = callback;
      if (callback->type == Lisple::RTValue::Type::SYMBOL)
      {
        resolved = ctx.lookup_value(callback->str());
      }

      if (!resolved || resolved->type != Lisple::RTValue::Type::FUNCTION)
      {
        throw Lisple::TypeError("Counter trigger callback must be callable");
      }

      Lisple::sptr_rtval_v args = {value};
      return resolved->exec().execute(ctx, args);
    }

    bool counter_stepped(const Pixils::State::Counter& before)
    {
      if (before.mode == Pixils::State::CounterMode::STOP && before.value == before.end)
      {
        return false;
      }

      return before.phase + 1 >= before.every;
    }
  } // namespace

  NATIVE_ADAPTER_IMPL(CounterAdapter,
                      State::Counter,
                      &HostType::COUNTER,
                      (MapKey::VALUE, value),
                      (MapKey::WRAPPED, wrapped),
                      (MapKey::START, start),
                      (MapKey::END, end),
                      (MapKey::EVERY, every),
                      (MapKey::PHASE, phase),
                      (MapKey::MODE, mode));

  NOBJ_PROP_GET__FIELD(CounterAdapter, value);
  NOBJ_PROP_GET__FIELD(CounterAdapter, wrapped);
  NOBJ_PROP_GET__FIELD(CounterAdapter, start);
  NOBJ_PROP_GET__FIELD(CounterAdapter, end);
  NOBJ_PROP_GET__FIELD(CounterAdapter, every);
  NOBJ_PROP_GET__FIELD(CounterAdapter, phase);

  NOBJ_PROP_GET(CounterAdapter, mode)
  {
    return Lisple::RTValue::keyword(Pixils::State::counter_mode_name(get_object().mode));
  }

  namespace Function
  {
    FUNC_IMPL(MakeCounter,
              SIG((FN_ARGS((&Lisple::Type::MAP)), EXEC_DISPATCH(&MakeCounter::exec_make))));

    Lisple::MapSchema counter_schema({},
                                     {{MapKey::START, &Lisple::Type::NUMBER},
                                      {MapKey::VALUE, &Lisple::Type::NUMBER},
                                      {MapKey::END, &Lisple::Type::NUMBER},
                                      {MapKey::EVERY, &Lisple::Type::NUMBER},
                                      {MapKey::MODE, &Lisple::Type::KEY}});

    EXEC_BODY(MakeCounter, exec_make)
    {
      auto opts = counter_schema.bind(ctx, *args[0]);

      Pixils::State::Counter counter{};
      counter.start = opts.i32(MapKey::START, 0);
      counter.value = opts.i32(MapKey::VALUE, counter.start);
      counter.end = opts.i32(MapKey::END, 10);
      counter.every = std::max(1, opts.i32(MapKey::EVERY, 1));
      counter.mode = parse_mode(opts.val(MapKey::MODE));

      return CounterAdapter::make_unique(counter);
    }

    FUNC_IMPL(AdvanceCounter,
              SIG((FN_ARGS((&HostType::COUNTER)),
                   EXEC_DISPATCH(&AdvanceCounter::exec_advance))));

    EXEC_BODY(AdvanceCounter, exec_advance)
    {
      return CounterAdapter::make_unique(
        Pixils::State::advance_counter(Lisple::obj<Pixils::State::Counter>(*args[0])));
    }

    FUNC_IMPL(
      AdvanceCounterAt,
      MULTI_SIG((FN_ARGS((&Lisple::Type::MAP), (&Lisple::Type::KEY)),
                 EXEC_DISPATCH(&AdvanceCounterAt::exec_advance_at)),
                (FN_ARGS((&Lisple::Type::MAP), (&Lisple::Type::KEY), (&Lisple::Type::MAP)),
                 EXEC_DISPATCH(&AdvanceCounterAt::exec_advance_at_with_triggers))));

    EXEC_BODY(AdvanceCounterAt, exec_advance_at)
    {
      Lisple::sptr_rtval_v full_args = {args[0], args[1], Lisple::RTValue::map({})};
      return exec_advance_at_with_triggers(ctx, full_args);
    }

    EXEC_BODY(AdvanceCounterAt, exec_advance_at_with_triggers)
    {
      auto current_counter = Lisple::Dict::get_property(args[0], args[1]);
      auto advanced_counter = Pixils::State::advance_counter(
        Lisple::obj<Pixils::State::Counter>(*current_counter));
      const bool stepped =
        counter_stepped(Lisple::obj<Pixils::State::Counter>(*current_counter));

      Lisple::sptr_rtval_v path = {args[1]};
      auto updated =
        Lisple::Dict::assoc_in(args[0], path, CounterAdapter::make_unique(advanced_counter));

      auto on_step = Lisple::Dict::get_property(*args[2], MapKey::ON_STEP);
      if (stepped && on_step && on_step->type != Lisple::RTValue::Type::NIL)
      {
        updated = invoke_callback(ctx, on_step, updated);
      }

      auto on_wrap = Lisple::Dict::get_property(*args[2], MapKey::ON_WRAP);
      if (advanced_counter.wrapped && on_wrap && on_wrap->type != Lisple::RTValue::Type::NIL)
      {
        updated = invoke_callback(ctx, on_wrap, updated);
      }

      return updated;
    }

    FUNC_IMPL(CounterValue,
              SIG((FN_ARGS((&HostType::COUNTER)),
                   EXEC_DISPATCH(&CounterValue::exec_value))));

    EXEC_BODY(CounterValue, exec_value)
    {
      return Lisple::RTValue::number(Lisple::obj<Pixils::State::Counter>(*args[0]).value);
    }

    FUNC_IMPL(CounterValueAt,
              SIG((FN_ARGS((&Lisple::Type::MAP), (&Lisple::Type::KEY)),
                   EXEC_DISPATCH(&CounterValueAt::exec_value_at))));

    EXEC_BODY(CounterValueAt, exec_value_at)
    {
      auto counter = Lisple::Dict::get_property(args[0], args[1]);
      return Lisple::RTValue::number(Lisple::obj<Pixils::State::Counter>(*counter).value);
    }
  } // namespace Function

  StateCounterNamespace::StateCounterNamespace()
    : Lisple::Namespace(std::string(NS__PIXILS__STATE__COUNTER))
  {
    values.emplace(FN__MAKE_COUNTER, Function::MakeCounter::make());
    values.emplace(FN__ADVANCE, Function::AdvanceCounter::make());
    values.emplace(FN__ADVANCE_AT, Function::AdvanceCounterAt::make());
    values.emplace(FN__VALUE, Function::CounterValue::make());
    values.emplace(FN__VALUE_AT, Function::CounterValueAt::make());
  }
} // namespace Pixils::Script
