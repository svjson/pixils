
#ifndef PIXILS__RUNTIME__EVENT_ROUTING_H
#define PIXILS__RUNTIME__EVENT_ROUTING_H

#include "pixils/ui/event.h"
#include <pixils/geom.h>
#include <pixils/ui/mouse_button.h>

#include <lisple/runtime/value.h>
#include <map>
#include <memory>
#include <vector>

namespace Lisple
{
  class Runtime;
}

namespace Pixils
{
  struct FrameEvents;
}

namespace Pixils::Runtime
{
  struct View;
  struct HookArguments;

  /**
   * Tracks which views are pressed and hovered across frames. Held by
   * EventRouter; all pointer fields use weak_ptr so that mode pops naturally
   * expire stale references without requiring explicit cleanup.
   */
  struct MouseState
  {
    using ViewChain = std::vector<std::weak_ptr<View>>;

    /**
     * One hit chain per currently held button: [0] = deepest hit view,
     * [1..n] = ancestors up to root. Keyed by the button that initiated
     * the press, so simultaneous multi-button presses are tracked independently.
     */
    std::map<UI::MouseButton, ViewChain> button_chains;

    /**
     * View currently under the cursor, and the full ancestor chain from it
     * up to root: [hovered, parent, ..., root]. Stored so handle_mouse_up
     * can propagate state changes back without rebuilding the chain.
     */
    std::weak_ptr<View> hovered;
    std::vector<std::weak_ptr<View>> hovered_chain;

    bool has_pressed() const { return !button_chains.empty(); }

    std::shared_ptr<View> pressed_by(UI::MouseButton btn) const
    {
      auto it = button_chains.find(btn);
      if (it == button_chains.end() || it->second.empty()) return nullptr;
      return it->second[0].lock();
    }
  };

  /**
   * Owns mouse state and dispatches all mouse events for the active mode tree.
   * Called once per frame from Session::update_mode(). Processes button events,
   * updates hover tracking, and runs the update traversal for the active mode.
   */
  struct EventRouter
  {
    MouseState mouse;

    /**
     * Run the full per-frame update for root: handle any pending mouse-up,
     * traverse the view tree (hover detection, boolean injection, update hooks,
     * enter/leave dispatch), then handle any pending mouse-down.
     * After this returns, root.state reflects all child updates.
     */
    void update(std::shared_ptr<View> root,
                FrameEvents& events,
                HookArguments& hook_args,
                Lisple::Runtime& rt);

    static std::vector<CustomEvent> process_events(View& receiver,
                                                   Lisple::sptr_rtval& view_ctx,
                                                   std::vector<CustomEvent>& events,
                                                   Lisple::Runtime& runtime);

   private:
    void handle_mouse_up(FrameEvents& events, HookArguments& hook_args, Lisple::Runtime& rt);

    void handle_mouse_down(std::shared_ptr<View> root,
                           FrameEvents& events,
                           HookArguments& hook_args,
                           Lisple::Runtime& rt);

    /**
     * Traverse the full view tree rooted at root. Updates hover tracking,
     * fires enter/leave hooks, injects hovered/pressed booleans, and calls
     * each view's update hook. Children are state-threaded through their
     * parent state map.
     */
    void traverse(std::shared_ptr<View> root,
                  FrameEvents& events,
                  HookArguments& hook_args,
                  Lisple::Runtime& rt);

    /**
     * Recursive helper called for every non-root view in the tree.
     * Extracts the view's state from parent, does hover/update work,
     * recurses into children, and merges the updated state back.
     */
    Lisple::sptr_rtval traverse_child(const std::shared_ptr<View>& view,
                                      const Lisple::sptr_rtval& parent_state,
                                      const Point& mouse_pos,
                                      HookArguments& hook_args,
                                      Lisple::Runtime& rt);

    /**
     * Update the engine-managed interaction flags on the view (hovered, pressed).
     * This does not write into the developer's state map.
     */
    void update_interaction(View& view, const Point& mouse_pos);

    /**
     * Depth-first hit test returning the deepest view whose bounds contain
     * (mx, my), together with the ancestor chain [deepest, ..., root].
     * Returns false if root itself is not hit.
     */
    static bool build_hit_chain(std::shared_ptr<View> view,
                                int mx,
                                int my,
                                std::vector<std::shared_ptr<View>>& chain);

    std::vector<CustomEvent> process_view_events();

    static Lisple::sptr_rtval invoke(
      const Lisple::sptr_rtval& fn,
      Lisple::sptr_rtval_v& args,
      Lisple::Runtime& rt,
      const Lisple::sptr_rtval& fallback = Lisple::Constant::NIL);
  };
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__EVENT_ROUTING_H */
