
#ifndef PIXILS__BINDING__UI_NAMESPACE_H
#define PIXILS__BINDING__UI_NAMESPACE_H

#include <pixils/ui/event.h>

#include <roo/exec.h>
#include <roo/host/object.h>
#include <roo/namespace.h>

namespace Roo
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct BindState;
}

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__UI = "pixils.ui";
  inline constexpr std::string_view FN__PIXILS__UI__BLUR_BANG = "blur!";
  inline constexpr std::string_view FN__PIXILS__UI__CHILDREN = "children";
  inline constexpr std::string_view FN__PIXILS__UI__FOCUS_BANG = "focus!";
  inline constexpr std::string_view FN__PIXILS__UI__FOCUS_FIRST_BANG = "focus-first!";
  inline constexpr std::string_view FN__PIXILS__UI__SET_UI_STATE_BANG = "set-ui-state!";
  inline constexpr std::string_view FN__PIXILS__UI__UPDATE_UI_STATE_BANG =
    "update-ui-state!";
  inline constexpr std::string_view FN__PIXILS__UI__STYLE_BANG = "style!";
  inline constexpr std::string_view FN__PIXILS__UI__THEME_VAR = "theme-var";

  namespace Function
  {
    /*!
     * @brief Creates a writable binding between child state and a path in its
     * parent state.
     *
     * An empty path binds the complete child state. The same form binds
     * application state when used under `:state` and UI state when used under
     * `:ui-state`.
     *
     * Usage:
     * @code
     * {:state {:value (pixils.ui/bind-state :selected-value)}}
     * @endcode
     *
     * | Arg     | Description                                  |
     * | ------- | -------------------------------------------- |
     * | path... | Keys identifying the path in the parent.     |
     *
     * @return A writable state-binding descriptor.
     *
     * @since 0.1.0
     * @see pixils.ui/consume-state
     * @see pixils.ui/project-state
     */
    FUNC(BindStateFn, bind_state);

    /*!
     * @brief Creates a read-only projection from a path in parent state into
     * child state.
     *
     * An empty path projects the complete parent state. Child changes are not
     * written back through this descriptor.
     *
     * Usage:
     * @code
     * {:ui-state {:options (pixils.ui/project-state :options)}}
     * @endcode
     *
     * | Arg     | Description                              |
     * | ------- | ---------------------------------------- |
     * | path... | Keys identifying the path in the parent. |
     *
     * @return A read-only state-binding descriptor.
     *
     * @since 0.1.0
     * @see pixils.ui/bind-state
     * @see pixils.ui/consume-state
     */
    FUNC(ProjectStateFn, project_state);

    /*!
     * @brief Creates an upward-only consumption from child state into a path
     * in parent state.
     *
     * The child owns and initializes the declared value. Once present, that
     * value is written to the parent path. Parent changes are never projected
     * into the child through this descriptor.
     *
     * Usage:
     * @code
     * {:ui-state {:measured-size (pixils.ui/consume-state :content-size)}}
     * @endcode
     *
     * | Arg     | Description                              |
     * | ------- | ---------------------------------------- |
     * | path... | Keys identifying the path in the parent. |
     *
     * @return An upward-only state-binding descriptor.
     *
     * @since 0.1.0
     * @see pixils.ui/bind-state
     * @see pixils.ui/project-state
     */
    FUNC(ConsumeStateFn, consume_state);

    /*!
     * @brief Appends a child description to a live view after the current
     * hook completes.
     *
     * Usage:
     * @code
     * (pixils.ui/append-child! ctx
     *                          {:mode 'ui/text
     *                           :id "status"
     *                           :state {:value "Ready"}})
     * @endcode
     *
     * | Arg    | Description                                  |
     * | ------ | -------------------------------------------- |
     * | target | Parent view or current hook context.         |
     * | child  | Child description map or text child value.   |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     * @see pixils.ui/remove-child!
     * @see pixils.ui/reconcile-children!
     */
    FUNC(AppendChildBangFunction, append_child);

    /*!
     * @brief Requests that focus be cleared from the current view or a
     * specified view.
     *
     * Usage:
     * @code
     * (pixils.ui/blur! ctx)
     * (pixils.ui/blur!)
     * @endcode
     *
     * | Arg    | Description                                  |
     * | ------ | -------------------------------------------- |
     * | target | Optional view or current hook context.       |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     * @see pixils.ui/focus!
     */
    FUNC(BlurBangFunction, blur);

    /*!
     * @brief Returns the direct live children of a view.
     *
     * Usage:
     * @code
     * (pixils.ui/children ctx)
     * => [#<View> #<View>]
     * @endcode
     *
     * | Arg    | Description                            |
     * | ------ | -------------------------------------- |
     * | target | Parent view or current hook context.   |
     *
     * @return A vector of direct child views.
     *
     * @since 0.1.0
     */
    FUNC(ChildrenFunction, children);

    /*!
     * @brief Emits a custom event from a view for ancestor event handlers.
     *
     * Usage:
     * @code
     * (pixils.ui/emit! ctx :list-box/change {:selected-indices [1]})
     * @endcode
     *
     * | Arg     | Description                                      |
     * | ------- | ------------------------------------------------ |
     * | target  | Source view or current hook context.             |
     * | event   | Keyword identifying the custom event.            |
     * | payload | Optional event payload; defaults to `nil`.       |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     */
    FUNC(EmitBangFunction, emit);

    /*!
     * @brief Requests keyboard focus for a view.
     *
     * Usage:
     * @code
     * (pixils.ui/focus! ctx)
     * @endcode
     *
     * | Arg    | Description                                |
     * | ------ | ------------------------------------------ |
     * | target | View or current hook context to focus.     |
     *
     * @return The requested view, or `nil` for a nil target.
     *
     * @since 0.1.0
     * @see pixils.ui/blur!
     * @see pixils.ui/focus-first!
     */
    FUNC(FocusBangFunction, focus);

    /*!
     * @brief Focuses the first eligible descendant of a view.
     *
     * When `container-mode` is supplied, the search begins at the first
     * descendant with that mode.
     *
     * Usage:
     * @code
     * (pixils.ui/focus-first! ctx 'ui/window-body)
     * @endcode
     *
     * | Arg            | Description                                      |
     * | -------------- | ------------------------------------------------ |
     * | target         | Root view or current hook context.               |
     * | container-mode | Optional symbol, keyword, or string mode name.   |
     *
     * @return The focused descendant view, or `nil` when none is eligible.
     *
     * @since 0.1.0
     * @see pixils.ui/focus!
     */
    FUNC(FocusFirstBangFunction, focus_first);

    /*!
     * @brief Marks an event so its default processing preserves current
     * keyboard focus.
     *
     * Usage:
     * @code
     * (pixils.ui/preserve-focus! event)
     * @endcode
     *
     * | Arg   | Description                    |
     * | ----- | ------------------------------ |
     * | event | Event whose focus is retained. |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     */
    FUNC(PreserveFocusBangFunction, preserve_focus);

    /*!
     * @brief Reconciles a view's direct children with an ordered collection
     * of named child descriptions.
     *
     * Matching IDs preserve their live views and state. Missing IDs create
     * children, absent IDs remove children, and retained children are reordered
     * to match the descriptions. A retained ID may not change its named mode or
     * component; use `pixils.ui/replace-child!` for explicit replacement.
     *
     * Usage:
     * @code
     * (pixils.ui/reconcile-children!
     *  ctx
     *  [{:mode 'row :id "first"}
     *   {:mode 'row :id "second"}])
     * @endcode
     *
     * | Arg      | Description                                         |
     * | -------- | --------------------------------------------------- |
     * | target   | Parent view or current hook context.                |
     * | children | Ordered vector with unique explicit child IDs.      |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     * @see pixils.ui/append-child!
     * @see pixils.ui/remove-child!
     * @see pixils.ui/replace-child!
     */
    FUNC(ReconcileChildrenBangFunction, reconcile_children);

    /*!
     * @brief Removes one direct child by ID after the current hook completes.
     *
     * Removing a view does not delete application data previously bound to
     * that view.
     *
     * Usage:
     * @code
     * (pixils.ui/remove-child! ctx "status")
     * @endcode
     *
     * | Arg      | Description                              |
     * | -------- | ---------------------------------------- |
     * | target   | Parent view or current hook context.     |
     * | child-id | String, symbol, or keyword child ID.     |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     * @see pixils.ui/append-child!
     * @see pixils.ui/reconcile-children!
     */
    FUNC(RemoveChildBangFunction, remove_child);

    /*!
     * @brief Explicitly replaces one direct child while retaining its child
     * ID and sibling position.
     *
     * Usage:
     * @code
     * (pixils.ui/replace-child! ctx "body" {:mode 'settings-body})
     * @endcode
     *
     * | Arg      | Description                                  |
     * | -------- | -------------------------------------------- |
     * | target   | Parent view or current hook context.         |
     * | child-id | String, symbol, or keyword child ID.         |
     * | child    | Replacement child description or text value. |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     * @see pixils.ui/reconcile-children!
     */
    FUNC(ReplaceChildBangFunction, replace_child);

    /*!
     * @brief Replaces the complete public UI state of a component view.
     *
     * The state passes through the component's declared UI-state models before
     * it is committed.
     *
     * Usage:
     * @code
     * (pixils.ui/set-ui-state! ctx {:selected-indices [1]})
     * @endcode
     *
     * | Arg      | Description                                  |
     * | -------- | -------------------------------------------- |
     * | target   | Component view or current hook context.      |
     * | ui-state | Complete candidate UI-state map.             |
     *
     * @return The target view, or `nil` for a nil target.
     *
     * @since 0.1.0
     * @see pixils.ui/update-ui-state!
     */
    FUNC(SetUIStateBangFunction, set_ui_state);

    /*!
     * @brief Applies an instance-specific runtime style to a live view.
     *
     * Usage:
     * @code
     * (pixils.ui/style! ctx {:width :fill :hidden false})
     * @endcode
     *
     * | Arg    | Description                              |
     * | ------ | ---------------------------------------- |
     * | target | View or current hook context.            |
     * | style  | Style value or style description map.    |
     *
     * @return The styled view, or `nil` for a nil target.
     *
     * @since 0.1.0
     */
    FUNC(StyleBangFunction, style);

    /*!
     * @brief Stops an event from continuing to ancestor handlers.
     *
     * Usage:
     * @code
     * (pixils.ui/stop-propagation! event)
     * @endcode
     *
     * | Arg   | Description                     |
     * | ----- | ------------------------------- |
     * | event | Event whose propagation stops.  |
     *
     * @return `nil`.
     *
     * @since 0.1.0
     */
    FUNC(StopPropagation, stop);

    /*!
     * @brief Resolves a theme variable for a view's effective theme and
     * selected variant.
     *
     * Usage:
     * @code
     * (pixils.ui/theme-var ctx :scrollbar-size 14)
     * => 14
     * @endcode
     *
     * | Arg      | Description                                     |
     * | -------- | ----------------------------------------------- |
     * | target   | View or current hook context.                   |
     * | key      | Theme-variable keyword or symbol.               |
     * | fallback | Optional value returned when the key is absent. |
     *
     * @return The resolved theme value, or `fallback` when absent.
     *
     * @since 0.1.0
     */
    FUNC(ActiveThemeVarFunction, theme_var);

    /*!
     * @brief Updates a component view's complete public UI state with a
     * function.
     *
     * The function receives current UI state followed by `args...`. Its result
     * passes through the component's declared UI-state models before commit.
     *
     * Usage:
     * @code
     * (pixils.ui/update-ui-state! ctx assoc :selected-indices [1])
     * @endcode
     *
     * | Arg       | Description                                      |
     * | --------- | ------------------------------------------------ |
     * | target    | Component view or current hook context.          |
     * | update-fn | Function returning the complete candidate map.   |
     * | args...   | Additional arguments passed to `update-fn`.      |
     *
     * @return The target view, or `nil` for a nil target.
     *
     * @since 0.1.0
     * @see pixils.ui/set-ui-state!
     */
    FUNC(UpdateUIStateBangFunction, update_ui_state);
  } // namespace Function

  NATIVE_ADAPTER(EventAdapter, Event);
  NATIVE_SUB_ADAPTER(EventAdapter,
                     (CustomEventAdapter, CustomEvent),
                     (event_key, source_mode, payload));
  NATIVE_SUB_ADAPTER(EventAdapter,
                     (MouseEventAdapter, MouseEvent),
                     (global_pos, local_pos, pass_depth));
  NATIVE_SUB_ADAPTER(MouseEventAdapter,
                     (MouseWheelEventAdapter, MouseWheelEvent),
                     (delta, x, y));
  NATIVE_SUB_ADAPTER(MouseEventAdapter,
                     (MouseButtonEventAdapter, MouseButtonEvent),
                     (button, click_count));
  NATIVE_SUB_ADAPTER(MouseButtonEventAdapter,
                     (DragEventAdapter, DragEvent),
                     (start_global_pos, start_local_pos, delta, total_delta, payload));
  NATIVE_SUB_ADAPTER(EventAdapter,
                     (KeyboardEventAdapter, KeyboardEvent),
                     (key, held_keys, match));
  NATIVE_ADAPTER(BindStateAdapter, Runtime::BindState);

  class UINamespace : public Roo::Namespace
  {
   public:
    UINamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__BINDING__UI_NAMESPACE_H */
