
#include "pixils/program.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/runtime/session.h>

#include <roo/runtime.h>
#include <roo/runtime/dict.h>

namespace Pixils
{
  namespace
  {
    Program& resolve_program(Roo::Runtime& runtime)
    {
      Roo::sptr_val programs = runtime.lookup(Script::ID__PIXILS__PROGRAMS);
      auto program_keys = Roo::Dict::map_keys(*programs);
      Roo::sptr_val program_key;

      if (program_keys.size() == 0)
      {
        runtime.eval("(pixils/defprogram program {})");
        program_key = Roo::symbol("program");
      }
      else
      {
        program_key = Roo::symbol(program_keys.front()->str());
      }

      auto program_val = Roo::Dict::get_property(programs, program_key);
      return Roo::obj<Program>(*program_val);
    }

    void ensure_initial_mode(Roo::Runtime& runtime, Program& program)
    {
      if (program.initial_mode != "") return;

      auto modes = runtime.lookup(Script::ID__PIXILS__MODES);
      auto mode_keys = Roo::Dict::map_keys(*modes);
      if (mode_keys.size() == 0) throw Roo::RooException("No modes defined");

      program.initial_mode = mode_keys.front()->str();
    }

  } // namespace

  Program::Program(const std::string& name,
                   Display& display,
                   const std::string& initial_mode)
    : name(name)
    , display(display)
    , initial_mode(initial_mode)
  {
  }

  const std::string& Program::get_name() const
  {
    return name;
  }

  Display& Program::get_display()
  {
    return display;
  }

  Program& load_program(Roo::Runtime& runtime, Runtime::Session& session)
  {
    auto& program = resolve_program(runtime);
    ensure_initial_mode(runtime, program);
    session.set_application_theme(program.theme, program.theme_variant);
    session.push_mode(program.initial_mode, Roo::Constant::NIL);
    return program;
  }
} // namespace Pixils
