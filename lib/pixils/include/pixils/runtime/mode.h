#ifndef PIXILS__RUNTIME__MODE_H
#define PIXILS__RUNTIME__MODE_H

#include <pixils/binding/pixils_namespace.h>
#include <pixils/ui/style.h>

#include <lisple/runtime/value.h>
#include <optional>
#include <string>
#include <vector>

namespace Pixils::Runtime
{
  struct ImageDependency
  {
    std::string resource_id;
    std::string file_name;
  };

  struct SoundDependency
  {
    std::string resource_id;
    std::string file_name;
  };

  struct ResourceDependencies
  {
    std::vector<ImageDependency> images;
    std::vector<SoundDependency> sounds;
  };

  struct ModeComposition
  {
    bool render = false;
    bool update = false;
  };

  /**
   * A slot in a layout tree: which mode to place there and how to identify it.
   * Sizing, positioning and per-instance hook/style overrides are carried in
   * `overrides` as a raw Lisple map; they are applied at build time in
   * build_view via apply_mode_overrides.
   * `id` is a sibling-unique key used to store this child's state in the parent
   * state map. Auto-generated as `mode-name-N` if not set explicitly.
   */
  struct ChildSlot
  {
    std::string mode_name;
    std::string id;
    Lisple::sptr_rtval initial_state;
    Lisple::sptr_rtval overrides = Lisple::Constant::NIL;
    Lisple::sptr_rtval state_binding = Lisple::Constant::NIL;
  };

  struct Mode
  {
    std::string name;
    ResourceDependencies resources;
    Lisple::sptr_rtval init = Lisple::Constant::NIL;
    Lisple::sptr_rtval update = Lisple::Constant::NIL;
    Lisple::sptr_rtval render = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_key_down = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_key_held = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_key_up = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_mouse_down = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_mouse_up = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_click = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_mouse_enter = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_mouse_leave = Lisple::Constant::NIL;
    Lisple::sptr_rtval on_mouse_motion = Lisple::Constant::NIL;
    std::map<std::string, Lisple::sptr_rtval> event_handlers;
    ModeComposition composition;
    std::vector<ChildSlot> children;
    std::optional<UI::Style> style = std::nullopt;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_H */
