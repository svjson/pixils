#ifndef PIXILS__UI__MOUSE_STATE_H
#define PIXILS__UI__MOUSE_STATE_H

#include <pixils/ui/mouse_button.h>

#include <map>
#include <memory>
#include <vector>

namespace Pixils::Runtime
{
  struct View;
}

namespace Pixils::UI
{
  /**
   * Tracks which views are pressed and hovered across frames. All pointer
   * fields use weak_ptr so that mode pops naturally expire stale references
   * without requiring explicit cleanup.
   */
  struct MouseState
  {
    using ViewChain = std::vector<std::weak_ptr<Pixils::Runtime::View>>;

    /**
     * One hit chain per currently held button: [0] = deepest hit view,
     * [1..n] = ancestors up to root. Keyed by the button that initiated
     * the press, so simultaneous multi-button presses are tracked independently.
     */
    std::map<MouseButton, ViewChain> button_chains;

    /**
     * View currently under the cursor, and the full ancestor chain from it
     * up to root: [hovered, parent, ..., root]. Stored so mouse-up handling
     * can propagate state changes back without rebuilding the chain.
     */
    std::weak_ptr<Pixils::Runtime::View> hovered;
    ViewChain hovered_chain;

    bool has_pressed() const { return !button_chains.empty(); }

    std::shared_ptr<Pixils::Runtime::View> pressed_by(MouseButton btn) const
    {
      auto it = button_chains.find(btn);
      if (it == button_chains.end() || it->second.empty()) return nullptr;
      return it->second[0].lock();
    }
  };

} // namespace Pixils::UI

#endif /* PIXILS__UI__MOUSE_STATE_H */
