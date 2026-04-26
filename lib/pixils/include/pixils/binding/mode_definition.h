
#ifndef PIXILS__BINDING__MODE_DEFINITION_H
#define PIXILS__BINDING__MODE_DEFINITION_H

#include <pixils/runtime/mode.h>

#include <lisple/runtime/value.h>
#include <vector>

namespace Lisple
{
  class Context;
}

namespace Pixils::Script
{
  /**
   * @brief Parse a Lisple sequence of child entry maps into ChildSlot objects.
   *
   * Each entry must carry a :mode symbol; :id and :state are optional.
   * The full entry map is stored as slot.overrides for deferred per-instance
   * hook and style application at build time.
   */
  std::vector<Runtime::ChildSlot> parse_child_slots(Lisple::Context& ctx,
                                                    const Lisple::sptr_rtval& children_val);

  /**
   * @brief Construct a Mode from a Lisple definition map.
   *
   * When the map contains :extends, the named base mode is looked up in the
   * mode registry and the returned Mode starts as a copy of it. Fields
   * explicitly present in the definition map override the copy; absent fields
   * retain the base value.
   *
   * Without :extends (and without a base pointer), absent fields use their
   * C++ zero-initialised defaults (NIL hooks, no style, no children).
   *
   * :on event handlers are always merged: derived handlers overlay base
   * handlers, with derived winning on key collision.
   */
  Runtime::Mode build_mode_from_definition(Lisple::Context& ctx,
                                           const Lisple::sptr_rtval& definition_map,
                                           const Runtime::Mode* base = nullptr);

} // namespace Pixils::Script

#endif /* PIXILS__BINDING__MODE_DEFINITION_H */
