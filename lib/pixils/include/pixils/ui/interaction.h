
#ifndef PIXILS__UI__INTERACTION_H
#define PIXILS__UI__INTERACTION_H

#include <pixils/ui/mouse_button.h>
#include <set>

namespace Pixils::UI
{
  /**
   * Interaction flags for a View.
   *
   * This informs style resolution when resolving style variants
   * (hover, pressed, etc).
   *
   * Updated each frame by the event routing mechanism.
   * `pressed` contains the buttons currently held down on this view.
   */
  struct InteractionState
  {
    bool hovered = false;
    std::set<MouseButton> pressed;
  };

} // namespace Pixils::UI

#endif /* PIXILS__UI__INTERACTION_H */
