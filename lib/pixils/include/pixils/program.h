
#ifndef PIXILS__PROGRAM_H
#define PIXILS__PROGRAM_H

#include "display.h"

#include <optional>
#include <string>

namespace Lisple
{
  class Runtime;
}

namespace Pixils
{
  namespace Runtime
  {
    struct Session;
  }

  class Program
  {
    std::string name;

   public:
    static constexpr int DEFAULT_TARGET_FRAME_RATE = 40;

    Display display;
    std::string initial_mode;
    std::optional<std::string> theme = std::nullopt;
    bool pointer_visible = true;
    int target_frame_rate = DEFAULT_TARGET_FRAME_RATE;

    Program(const std::string& name, Display& display, const std::string& initial_mode);

    const std::string& get_name() const;
    Display& get_display();
    void set_display(Display& display);
  };

  Program& load_program(Lisple::Runtime& runtime, Runtime::Session& session);
} // namespace Pixils

#endif /* PROGRAM_H */
