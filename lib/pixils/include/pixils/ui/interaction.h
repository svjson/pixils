
#ifndef PIXILS__UI__INTERACTION_H
#define PIXILS__UI__INTERACTION_H

namespace Pixils::UI
{
  /**
   * Interaction flags for a View.
   *
   * This informs style resolution when resolving style variants
   * (hover, pressed, etc).
   *
   * Updated each frame by the event routing mechanism.
   */
  struct InteractionState
  {
    bool hovered = false;
    bool pressed = false;
  };

} // namespace Pixils::UI

#endif /* PIXILS__UI__INTERACTION_H */
