#ifndef PIXILS__RUNTIME__MODE_H
#define PIXILS__RUNTIME__MODE_H

#include "pixils/binding/pixils_namespace.h"

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

  struct ResourceDependencies
  {
    std::vector<ImageDependency> images;
  };

  struct ModeComposition
  {
    bool render = false;
    bool update = false;
  };

  /** Constraint on one dimension of a layout child. */
  struct DimensionConstraint
  {
    enum class Kind
    {
      FIXED,    // exact pixel count
      FILL,     // take remaining space
      RELATIVE, // percentage of parent
    };

    Kind kind = Kind::FILL;
    int value = 0; // pixels for FIXED, 0-100 for RELATIVE; unused for FILL

    static DimensionConstraint fixed(int pixels) { return {Kind::FIXED, pixels}; }
    static DimensionConstraint fill() { return {Kind::FILL, 0}; }
    static DimensionConstraint relative(int percent) { return {Kind::RELATIVE, percent}; }
  };

  /** Per-side margin around a layout child. Stored but not yet applied in layout calculations. */
  struct Margin
  {
    int top = 0;
    int right = 0;
    int bottom = 0;
    int left = 0;
  };

  enum class LayoutDirection
  {
    COLUMN,
    ROW,
  };

  /**
   * A slot in a layout tree: which mode to place there and how to size it.
   * `id` is a sibling-unique key used to store this child's state in the
   * parent state map. Auto-generated as `mode-name-N` if not set explicitly.
   */
  struct ChildSlot
  {
    std::string mode_name;
    std::string id;
    std::optional<DimensionConstraint> width;
    std::optional<DimensionConstraint> height;
    Margin margin;
  };

  struct Mode
  {
    std::string name;
    ResourceDependencies resources;
    Lisple::sptr_rtval init;
    Lisple::sptr_rtval update;
    Lisple::sptr_rtval render;
    ModeComposition composition;
    LayoutDirection layout_direction = LayoutDirection::COLUMN;
    std::vector<ChildSlot> children;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__MODE_H */
