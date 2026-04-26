
#ifndef PIXILS__UI__MOUSE_BUTTON_H
#define PIXILS__UI__MOUSE_BUTTON_H

#include <cstdint>
#include <string_view>

namespace Pixils::UI
{
  enum class MouseButton : uint8_t
  {
    NONE = 0,
    LEFT = 1,
    RIGHT = 2,
    MIDDLE = 3,
  };

  inline const char* mouse_button_name(MouseButton btn)
  {
    switch (btn)
    {
    case MouseButton::LEFT:
      return "left";
    case MouseButton::RIGHT:
      return "right";
    case MouseButton::MIDDLE:
      return "middle";
    default:
      return "";
    }
  }

  inline MouseButton mouse_button_from_name(std::string_view name)
  {
    if (name == "left") return MouseButton::LEFT;
    if (name == "right") return MouseButton::RIGHT;
    if (name == "middle") return MouseButton::MIDDLE;
    return MouseButton::NONE;
  }

} // namespace Pixils::UI

#endif /* PIXILS__UI__MOUSE_BUTTON_H */
