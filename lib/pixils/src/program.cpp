
#include "pixils/program.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/runtime/session.h>

#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>

namespace Pixils
{
  namespace
  {
    Lisple::sptr_val theme_names_to_value(const std::vector<std::string>& theme_names)
    {
      if (theme_names.size() == 1)
      {

        return Lisple::symbol(theme_names[0]);
      }

      std::vector<Lisple::sptr_val> values;
      values.reserve(theme_names.size());
      for (const auto& theme_name : theme_names)
      {
        values.push_back(Lisple::symbol(theme_name));
      }
      return Lisple::vector(values);
    }

    Program& resolve_program(Lisple::Runtime& runtime)
    {
      Lisple::sptr_val programs = runtime.lookup(Script::ID__PIXILS__PROGRAMS);
      auto program_keys = Lisple::Dict::map_keys(*programs);
      Lisple::sptr_val program_key;

      if (program_keys.size() == 0)
      {
        runtime.eval("(pixils/defprogram program {})");
        program_key = Lisple::symbol("program");
      }
      else
      {
        program_key = Lisple::symbol(program_keys.front()->str());
      }

      auto program_val = Lisple::Dict::get_property(programs, program_key);
      return Lisple::obj<Program>(*program_val);
    }

    void ensure_initial_mode(Lisple::Runtime& runtime, Program& program)
    {
      if (program.initial_mode != "") return;

      auto modes = runtime.lookup(Script::ID__PIXILS__MODES);
      auto mode_keys = Lisple::Dict::map_keys(*modes);
      if (mode_keys.size() == 0) throw Lisple::LispleException("No modes defined");

      program.initial_mode = mode_keys.front()->str();
    }

    Lisple::sptr_val program_root_overrides(const Program& program)
    {
      if (!program.theme && !program.theme_variant) return Lisple::Constant::NIL;

      std::vector<Lisple::sptr_val> values;
      if (program.theme)
      {
        values.push_back(Lisple::keyword("theme"));
        values.push_back(theme_names_to_value(*program.theme));
      }
      if (program.theme_variant)
      {
        values.push_back(Lisple::keyword("theme-variant"));
        values.push_back(Lisple::keyword(*program.theme_variant));
      }
      return Lisple::map(values);
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

  Program& load_program(Lisple::Runtime& runtime, Runtime::Session& session)
  {
    auto& program = resolve_program(runtime);
    ensure_initial_mode(runtime, program);
    session.push_mode(program.initial_mode,
                      Lisple::Constant::NIL,
                      program_root_overrides(program));
    return program;
  }
} // namespace Pixils
