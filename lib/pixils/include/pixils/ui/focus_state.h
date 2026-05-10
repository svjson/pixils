#ifndef PIXILS__UI__FOCUS_STATE_H
#define PIXILS__UI__FOCUS_STATE_H

#include <memory>
#include <vector>

namespace Pixils::Runtime
{
  struct View;
}

namespace Pixils::UI
{
  /**
   * Tracks the currently focused view and its ancestor chain from leaf to root.
   * Stored as weak_ptrs so stale focus drops naturally when views are rebuilt.
   */
  struct FocusState
  {
    using ViewChain = std::vector<std::weak_ptr<Pixils::Runtime::View>>;

    std::weak_ptr<Pixils::Runtime::View> focused;
    ViewChain focus_chain;

    bool has_focus() const { return !focused.expired(); }

    void clear()
    {
      focused.reset();
      focus_chain.clear();
    }
  };

} // namespace Pixils::UI

#endif /* PIXILS__UI__FOCUS_STATE_H */
