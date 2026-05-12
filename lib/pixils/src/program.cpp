
#include "pixils/program.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/runtime/session.h>

#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>

namespace Pixils
{
  namespace
  {
    Lisple::sptr_rtval theme_names_to_value(const std::vector<std::string>& theme_names)
    {
      if (theme_names.size() == 1)
      {

        return Lisple::RTValue::symbol(theme_names[0]);
      }

      std::vector<Lisple::sptr_rtval> values;
      values.reserve(theme_names.size());
      for (const auto& theme_name : theme_names)
      {
        values.push_back(Lisple::RTValue::symbol(theme_name));
      }
      return Lisple::RTValue::vector(values);
    }

    Program& resolve_program(Lisple::Runtime& runtime)
    {
      Lisple::sptr_rtval programs = runtime.lookup_value(Script::ID__PIXILS__PROGRAMS);
      auto program_keys = Lisple::Dict::map_keys(*programs);
      Lisple::sptr_rtval program_key;

      if (program_keys.size() == 0)
      {
        runtime.eval("(pixils/defprogram program {})");
        program_key = Lisple::RTValue::symbol("program");
      }
      else
      {
        program_key = Lisple::RTValue::symbol(program_keys.front()->str());
      }

      auto program_val = Lisple::Dict::get_property(programs, program_key);
      return Lisple::obj<Program>(*program_val);
    }

    void ensure_initial_mode(Lisple::Runtime& runtime, Program& program)
    {
      if (program.initial_mode != "") return;

      auto modes = runtime.lookup_value(Script::ID__PIXILS__MODES);
      auto mode_keys = Lisple::Dict::map_keys(*modes);
      if (mode_keys.size() == 0) throw Lisple::LispleException("No modes defined");

      program.initial_mode = mode_keys.front()->str();
    }

    Lisple::sptr_rtval program_root_overrides(const Program& program)
    {
      if (!program.theme) return Lisple::Constant::NIL;

      return Lisple::RTValue::map(
        {Lisple::RTValue::keyword("theme"), theme_names_to_value(*program.theme)});
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
