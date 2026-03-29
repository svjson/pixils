
#include "pixils/program.h"

namespace Pixils
{
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
} // namespace Pixils
