
#ifndef PIXILS__RUNTIME__EVENT_ROUTING_H
#define PIXILS__RUNTIME__EVENT_ROUTING_H

#include "pixils/ui/event.h"
#include <pixils/geom.h>
#include <pixils/ui/mouse_state.h>

#include <lisple/runtime/value.h>
#include <memory>

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
   * Dispatches all mouse events for the active mode tree using the caller's
   * persistent mouse state. Called once per frame from Session::update_mode().
   * Processes button events, updates hover tracking, and runs the update
   * traversal for the active mode.
   */
  struct EventRouter
  {
    /**
     * Run the full per-frame update for root: handle pending mouse-up/down
     * edges, traverse the view tree (hover detection, interaction injection,
     * update hooks, enter/leave dispatch), then fire any mouse-motion hooks.
     * After this returns, root.state reflects all bound child updates.
     */
    void update(std::shared_ptr<View> root,
                UI::MouseState& mouse_state,
                FrameEvents& events,
                HookArguments& hook_args,
                Lisple::Runtime& rt);

    /**
     * For each event: if receiver has a matching handler, invoke it and merge
     * the updated receiver state back into parent_state immediately. Unhandled
     * events (no handler, or propagation not stopped) are returned so they can
     * bubble to the next ancestor.
     */
    static std::vector<CustomEvent> process_events(View& receiver,
                                                   Lisple::sptr_rtval* parent_state,
                                                   Lisple::sptr_rtval& view_ctx,
                                                   std::vector<CustomEvent>& events,
                                                   Lisple::Runtime& runtime,
                                                   bool* receiver_state_updated = nullptr);

   private:
    void handle_mouse_up(UI::MouseState& mouse_state,
                         FrameEvents& events,
                         HookArguments& hook_args,
                         Lisple::Runtime& rt);

    void handle_mouse_down(std::shared_ptr<View> root,
                           UI::MouseState& mouse_state,
                           FrameEvents& events,
                           HookArguments& hook_args,
                           Lisple::Runtime& rt);

    /** Fire on_mouse_motion on the current hovered chain with inline state threading. */
    void handle_mouse_motion(UI::MouseState& mouse_state,
                             FrameEvents& events,
                             HookArguments& hook_args,
                             Lisple::Runtime& rt);

    /**
     * Traverse the full view tree rooted at root. Updates hover tracking,
     * fires enter/leave hooks, injects hovered/pressed booleans, and calls
     * each view's update hook. Per-view recursion is delegated to
     * traverse_child().
     */
    void traverse(std::shared_ptr<View> root,
                  UI::MouseState& mouse_state,
                  FrameEvents& events,
                  HookArguments& hook_args,
                  Lisple::Runtime& rt);

    /**
     * Recursive helper for one view subtree. When parent_state is non-null,
     * the view state is extracted from the parent before update work and
     * merged back after children complete. Passing nullptr is valid for the
     * active root, which already owns its state directly.
     */
    void traverse_child(const std::shared_ptr<View>& view,
                        const UI::MouseState& mouse_state,
                        Lisple::sptr_rtval* parent_state,
                        const Point& mouse_pos,
                        HookArguments& hook_args,
                        Lisple::Runtime& rt);

    /**
     * Update the engine-managed interaction flags on the view (hovered, pressed).
     * This does not write into the developer's state map.
     */
    void update_interaction(View& view,
                            const Point& mouse_pos,
                            const UI::MouseState& mouse_state);
  };
} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__EVENT_ROUTING_H */
