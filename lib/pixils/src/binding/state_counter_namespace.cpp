#include <pixils/binding/state_counter_namespace.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exception.h>
#include <roo/host/schema.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

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

    Pixils::State::CounterMode parse_mode(const Roo::sptr_val& mode_val)
    {
      if (!mode_val || mode_val->type == Roo::Value::Type::NIL)
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

    Roo::sptr_val invoke_callback(Roo::Context& ctx,
                                     const Roo::sptr_val& callback,
                                     const Roo::sptr_val& value)
    {
      if (!callback || callback->type == Roo::Value::Type::NIL)
      {
        return value;
      }

      Roo::sptr_val resolved = callback;
      if (callback->type == Roo::Value::Type::SYMBOL)
      {
        resolved = ctx.lookup(callback->str());
      }

      if (!resolved || resolved->type != Roo::Value::Type::FUNCTION)
      {
        throw Roo::TypeError("Counter trigger callback must be callable");
      }

      Roo::sptr_val_v args = {value};
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
    return Roo::keyword(Pixils::State::counter_mode_name(get_object().mode));
  }

  namespace Function
  {
    FUNC_IMPL(MakeCounter,
              SIG((FN_ARGS((&Roo::Type::MAP)), EXEC_DISPATCH(&MakeCounter::exec_make))));

    Roo::MapSchema counter_schema({},
                                     {{MapKey::START, &Roo::Type::NUMBER},
                                      {MapKey::VALUE, &Roo::Type::NUMBER},
                                      {MapKey::END, &Roo::Type::NUMBER},
                                      {MapKey::EVERY, &Roo::Type::NUMBER},
                                      {MapKey::MODE, &Roo::Type::KEYWORD}});

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
        Pixils::State::advance_counter(Roo::obj<Pixils::State::Counter>(*args[0])));
    }

    FUNC_IMPL(AdvanceCounterAt,
              MULTI_SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::KEYWORD)),
                         EXEC_DISPATCH(&AdvanceCounterAt::exec_advance_at)),
                        (FN_ARGS((&Roo::Type::MAP),
                                 (&Roo::Type::KEYWORD),
                                 (&Roo::Type::MAP)),
                         EXEC_DISPATCH(&AdvanceCounterAt::exec_advance_at_with_triggers))));

    EXEC_BODY(AdvanceCounterAt, exec_advance_at)
    {
      Roo::sptr_val_v full_args = {args[0], args[1], Roo::map({})};
      return exec_advance_at_with_triggers(ctx, full_args);
    }

    EXEC_BODY(AdvanceCounterAt, exec_advance_at_with_triggers)
    {
      auto current_counter = Roo::Dict::get_property(args[0], args[1]);
      auto advanced_counter = Pixils::State::advance_counter(
        Roo::obj<Pixils::State::Counter>(*current_counter));
      const bool stepped =
        counter_stepped(Roo::obj<Pixils::State::Counter>(*current_counter));

      Roo::sptr_val_v path = {args[1]};
      auto updated =
        Roo::Dict::assoc_in(args[0], path, CounterAdapter::make_unique(advanced_counter));

      auto on_step = Roo::Dict::get_property(*args[2], MapKey::ON_STEP);
      if (stepped && on_step && on_step->type != Roo::Value::Type::NIL)
      {
        updated = invoke_callback(ctx, on_step, updated);
      }

      auto on_wrap = Roo::Dict::get_property(*args[2], MapKey::ON_WRAP);
      if (advanced_counter.wrapped && on_wrap && on_wrap->type != Roo::Value::Type::NIL)
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
      return Roo::number(Roo::obj<Pixils::State::Counter>(*args[0]).value);
    }

    FUNC_IMPL(CounterValueAt,
              SIG((FN_ARGS((&Roo::Type::MAP), (&Roo::Type::KEYWORD)),
                   EXEC_DISPATCH(&CounterValueAt::exec_value_at))));

    EXEC_BODY(CounterValueAt, exec_value_at)
    {
      auto counter = Roo::Dict::get_property(args[0], args[1]);
      return Roo::number(Roo::obj<Pixils::State::Counter>(*counter).value);
    }
  } // namespace Function

  StateCounterNamespace::StateCounterNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__STATE__COUNTER))
  {
    values.emplace(FN__MAKE_COUNTER, Function::MakeCounter::make());
    values.emplace(FN__ADVANCE, Function::AdvanceCounter::make());
    values.emplace(FN__ADVANCE_AT, Function::AdvanceCounterAt::make());
    values.emplace(FN__VALUE, Function::CounterValue::make());
    values.emplace(FN__VALUE_AT, Function::CounterValueAt::make());
  }
} // namespace Pixils::Script
