#ifndef PIXILS__RUNTIME__VIEW_DEFINITION_H
#define PIXILS__RUNTIME__VIEW_DEFINITION_H

#include <pixils/color.h>
#include <pixils/ui/drag.h>
#include <pixils/ui/style.h>

#include <map>
#include <memory>
#include <optional>
#include <roo/runtime/value.h>
#include <string>
#include <vector>

namespace Pixils::Runtime
{
  struct Component;
  struct Mode;

  struct ImageDependency
  {
    std::string resource_id;
    std::string file_name;
    std::optional<Color> transparency_color = std::nullopt;
  };

  struct SoundDependency
  {
    std::string resource_id;
    std::string file_name;
  };

  struct MusicDependency
  {
    std::string resource_id;
    std::string file_name;
  };

  struct FontDependency
  {
    std::string resource_id;
    std::string file_name;
  };

  struct ResourceDependencies
  {
    std::vector<ImageDependency> images;
    std::vector<SoundDependency> sounds;
    std::vector<MusicDependency> music;
    std::vector<FontDependency> fonts;
  };

  struct StyleLayer
  {
    std::optional<UI::Style> style = std::nullopt;
    Roo::sptr_val source = Roo::Constant::NIL;
  };

  /**
   * A slot in a layout tree: which definition to place there and how to
   * identify it. Named child slots resolve either `mode_name` or
   * `component_name` from its corresponding registry and carry sizing,
   * positioning and per-instance hook/style overrides in `overrides`.
   * Anonymous child slots carry `anonymous_mode` directly and do not require a
   * registry entry.
   *
   * `id` is a sibling-unique key used to store this child's state in the parent
   * state map. Auto-generated as `mode-name-N` or `anonymous-N` if not set
   * explicitly.
   */
  struct ChildSlot
  {
    std::string mode_name;
    std::string component_name;
    std::string id;
    std::shared_ptr<Mode> anonymous_mode = nullptr;
    Roo::sptr_val initial_state;
    Roo::sptr_val raw_state = Roo::Constant::NIL;
    Roo::sptr_val initial_ui_state = Roo::Constant::NIL;
    Roo::sptr_val initial_component_state = Roo::Constant::NIL;
    bool has_initial_ui_state = false;
    bool has_component_state = false;
    Roo::sptr_val overrides = Roo::Constant::NIL;
    Roo::sptr_val state_binding = Roo::Constant::NIL;
    Roo::sptr_val component_state_binding = Roo::Constant::NIL;
    std::optional<std::string> state_policy = std::nullopt;
  };

  struct ViewDefinition
  {
    std::string name;
    std::vector<std::string> selector_modes;
    std::vector<std::string> class_names;
    bool focusable = false;
    ResourceDependencies resources;
    Roo::sptr_val init = Roo::Constant::NIL;
    Roo::sptr_val update = Roo::Constant::NIL;
    Roo::sptr_val after_layout = Roo::Constant::NIL;
    Roo::sptr_val content_size = Roo::Constant::NIL;
    Roo::sptr_val render = Roo::Constant::NIL;
    Roo::sptr_val action_map = Roo::Constant::NIL;
    Roo::sptr_val on_key_down = Roo::Constant::NIL;
    Roo::sptr_val on_key_held = Roo::Constant::NIL;
    Roo::sptr_val on_key_up = Roo::Constant::NIL;
    Roo::sptr_val on_mouse_down = Roo::Constant::NIL;
    Roo::sptr_val on_mouse_up = Roo::Constant::NIL;
    Roo::sptr_val on_click = Roo::Constant::NIL;
    Roo::sptr_val on_double_click = Roo::Constant::NIL;
    Roo::sptr_val on_mouse_enter = Roo::Constant::NIL;
    Roo::sptr_val on_mouse_leave = Roo::Constant::NIL;
    Roo::sptr_val on_mouse_motion = Roo::Constant::NIL;
    Roo::sptr_val on_mouse_wheel = Roo::Constant::NIL;
    Roo::sptr_val on_drag_start = Roo::Constant::NIL;
    Roo::sptr_val on_drag = Roo::Constant::NIL;
    Roo::sptr_val on_drag_end = Roo::Constant::NIL;
    Roo::sptr_val on_drop = Roo::Constant::NIL;
    std::map<std::string, Roo::sptr_val> event_handlers;
    std::vector<ChildSlot> children;
    std::optional<UI::DragPolicy> drag = std::nullopt;
    std::optional<UI::Style> style = std::nullopt;
    std::vector<StyleLayer> style_layers;
    std::optional<UI::Style> runtime_style = std::nullopt;
    Roo::sptr_val runtime_style_source = Roo::Constant::NIL;
    std::optional<std::vector<std::string>> theme = std::nullopt;
    std::optional<std::string> theme_variant = std::nullopt;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__VIEW_DEFINITION_H */
