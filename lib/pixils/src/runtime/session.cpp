
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/pixils_namespace.h>

#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  void HookArguments::update_state(const Lisple::sptr_rtval& state)
  {
    this->init_args[0] = state;
    this->update_args[0] = state;
    this->render_args[0] = state;
  }

  Session::Session(Lisple::Runtime& lisple_runtime,
                   Asset::Registry& assets,
                   const HookArguments& hook_args)
    : lisple_runtime(lisple_runtime)
    , assets(assets)
    , mode_stack(lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK),
                 lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK_MESSAGES))
    , modes(lisple_runtime.lookup_value(Script::ID__PIXILS__MODES))
    , hook_args(hook_args)
  {
  }

  void Session::pop_mode()
  {
    if (mode_stack.size() > 1)
    {
      mode_stack.pop();

      auto [mode, mode_state] = mode_stack.peek();
      this->active_mode.mode_index = mode_stack.size() - 1;
      this->active_mode.render_fun = mode->render->to_string();
      this->active_mode.update_fun = mode->update->to_string();
      this->active_mode.init_fun = mode->init->to_string();
      this->active_mode.state = mode_state;

      this->hook_args.update_state(mode_state);
    }
  }

  void Session::push_mode(const Lisple::sptr_rtval& mode, const Lisple::sptr_rtval& state)
  {
    this->mode_stack.push(mode, state);

    auto& mode_obj = Lisple::obj<Mode>(*mode);

    this->active_mode.mode_index = mode_stack.size() - 1;
    this->active_mode.render_fun = mode_obj.render->to_string();
    this->active_mode.update_fun = mode_obj.update->to_string();
    this->active_mode.init_fun = mode_obj.init->to_string();
    this->active_mode.state = state;

    if (!this->assets.is_loaded(mode_obj.name))
    {
      this->assets.load(mode_obj.name, mode_obj.resources);
    }

    this->hook_args.update_state(state);

    this->init_mode();
  }

  void Session::push_mode(const std::string& mode_name, const Lisple::sptr_rtval& state)
  {
    auto mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(mode_name));
    this->push_mode(mode, state);
  }

  void Session::process_messages()
  {
    Lisple::sptr_rtval_v messages = mode_stack.drain_messages();

    for (auto& message : messages)
    {
      std::string type =
        Lisple::Dict::get_property(message, Lisple::RTValue::keyword("type"))->str();

      if (type == "push")
      {
        mode_stack.update_state(active_mode.state);
        push_mode(
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("mode"))->str(),
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("state")));
      }
      else if (type == "pop")
      {
        pop_mode();
      }
    }
  }

  void Session::init_mode()
  {
    if (!this->active_mode.init_fun.empty())
    {
      this->active_mode.state =
        lisple_runtime.invoke(this->active_mode.init_fun, this->hook_args.init_args);
      this->hook_args.update_state(this->active_mode.state);
    }
  }

  void Session::update_mode()
  {
    Lisple::sptr_rtval updated_state =
      this->lisple_runtime.invoke(this->active_mode.update_fun, this->hook_args.update_args);
    this->hook_args.update_state(updated_state);
    this->active_mode.state = updated_state;
  }

  void Session::render_mode()
  {

    auto render_stack = mode_stack.get_render_stack();

    for (size_t i = render_stack.size() - 1; i > 0; i--)
    {
      auto [mode, mode_state] = render_stack[i];

      Lisple::sptr_rtval_v rargs = this->hook_args.render_args;
      rargs[0] = mode_state;

      lisple_runtime.invoke(mode->render->to_string(), rargs);
    }

    lisple_runtime.invoke(this->active_mode.render_fun, this->hook_args.render_args);
  }

} // namespace Pixils::Runtime
