
#ifndef PIXILS__RUNTIME__VIEW_H
#define PIXILS__RUNTIME__VIEW_H

#include "pixils/ui/event.h"
#include <pixils/runtime/component.h>
#include <pixils/runtime/mode.h>
#include <pixils/ui/interaction.h>
#include <pixils/ui/style_view.h>
#include <pixils/ui/theme.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <roo/runtime/value.h>

namespace Roo
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct QueuedChildReplacement
  {
    std::string child_id;
    ChildSlot child_slot;
  };

  struct QueuedChildAppend
  {
    ChildSlot child_slot;
  };

  struct QueuedChildRemoval
  {
    std::string child_id;
  };

  /**
   * Live instance of a view definition. Holds the resolved shared definition
   * pointer, optional concrete mode/component pointer, Roo state, layout bounds,
   * and any nested child views. For layout children, `id` is the key under
   * which this view's state is stored in the parent state map.
   */
  struct View
  {
    std::string id;
    View* parent = nullptr;
    Roo::sptr_val state_binding = Roo::Constant::NIL;
    ViewDefinition* definition = nullptr;
    Mode* mode = nullptr;
    Component* component = nullptr;
    UI::InteractionState interaction;
    /**
     * Owns a per-instance copy of the mode when push-time or
     * child-slot overrides are present. `mode` points here instead of
     * into the shared registry. unique_ptr so that the pointer
     * remains valid when View is moved.
     */
    std::unique_ptr<Mode> owned_mode;
    std::unique_ptr<Component> owned_component;
    Roo::sptr_val component_state_binding = Roo::Constant::NIL;
    Roo::sptr_val ui_state_binding = Roo::Constant::NIL;
    Roo::sptr_val state = Roo::Constant::NIL;
    Roo::sptr_val initial_state = Roo::Constant::NIL;
    Roo::sptr_val ui_state = Roo::Constant::NIL;
    Roo::sptr_val initial_ui_state = Roo::Constant::NIL;
    std::string state_policy = "shared";
    Rect bounds = {0, 0, 0, 0};
    Rect external_bounds = {0, 0, 0, 0};
    Rect visual_bounds = {0, 0, 0, 0};
    int visual_scale = 1;
    std::optional<UI::Theme> inherited_theme = std::nullopt;
    UI::StyleView style_view;
    UI::Theme effective_theme;
    UI::Style effective_style;
    std::uint64_t state_generation = 1;
    std::uint64_t interaction_generation = 1;
    std::uint64_t children_generation = 1;
    std::uint64_t style_generation = 1;
    std::uint64_t subtree_generation = 1;
    struct NaturalContentSizeCache
    {
      bool valid = false;
      std::optional<int> available_width = std::nullopt;
      std::optional<int> available_height = std::nullopt;
      std::uint64_t style_generation = 0;
      std::size_t subtree_signature = 0;
      std::optional<Dimension> value = std::nullopt;
    };
    NaturalContentSizeCache natural_content_size_cache;
    struct LayoutCache
    {
      bool valid = false;
      Rect requested_bounds = {0, 0, 0, 0};
      std::uint64_t style_generation = 0;
      std::uint64_t subtree_generation = 0;
      std::uint64_t font_generation = 0;
    };
    LayoutCache layout_cache;
    std::vector<std::shared_ptr<View>> children;
    std::vector<CustomEvent> emitted_events;
    std::vector<QueuedChildReplacement> pending_child_replacements;
    std::vector<QueuedChildAppend> pending_child_appends;
    std::vector<QueuedChildRemoval> pending_child_removals;

    void set_parent(View* parent);
    void touch_subtree_generation();
    void mark_state_changed();
    void mark_interaction_changed();
    void mark_children_changed();
    void mark_style_changed();
    bool set_state_if_changed(const Roo::sptr_val& next_state);
    bool set_ui_state_if_changed(const Roo::sptr_val& next_state);
    bool set_state_from_child_bindings(const Roo::sptr_val& next_state,
                                       Roo::Runtime& runtime);
    bool set_ui_state_from_child_bindings(const View& child, Roo::Runtime& runtime);
    void sync_ui_state_from_child_bindings(const Roo::sptr_val& previous_state,
                                           const Roo::sptr_val& next_state,
                                           Roo::Runtime& runtime);
    bool has_component_ui_state() const;
    void seed_ui_state_from_shared_state_policy(Roo::Runtime& runtime);
    void sync_state_from_shared_ui_state_policy();
    void emit_event(const CustomEvent& event);
    void drain_events(std::vector<CustomEvent>& collected);
    void queue_replace_child(const std::string& child_id, ChildSlot child_slot);
    void queue_append_child(ChildSlot child_slot);
    void queue_remove_child(const std::string& child_id);
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__VIEW_H */
