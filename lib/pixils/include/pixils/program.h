
#ifndef PIXILS__PROGRAM_H
#define PIXILS__PROGRAM_H

#include "display.h"

#include <string>

namespace Pixils
{
  class Program
  {
    std::string name;

   public:
    Display display;
    std::string initial_mode;
    bool pointer_visible = true;

    Program(const std::string& name, Display& display, const std::string& initial_mode);

    const std::string& get_name() const;
    Display& get_display();
    void set_display(Display& display);
  };
} // namespace Pixils

#endif /* PROGRAM_H */
