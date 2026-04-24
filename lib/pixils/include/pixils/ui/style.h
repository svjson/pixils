
#ifndef PIXILS__UI__STYLE_H
#define PIXILS__UI__STYLE_H

#include <pixils/geom.h>

#include <SDL2/SDL_render.h>
#include <memory>
#include <optional>

namespace Lisple
{
  class RTValue;
  using sptr_rtval = std::shared_ptr<RTValue>;
} // namespace Lisple

namespace Pixils::UI
{
  /** How a child is positioned within its parent's layout. */
  enum class PositionMode
  {
    FLOW,     // participates in parent flow (default)
    ABSOLUTE, // positioned at :top/:left relative to parent content rect
  };

  /** Direction in which a mode lays out its children. */
  enum class LayoutDirection
  {
    COLUMN, // top to bottom (default)
    ROW,    // left to right
  };

  struct Style
  {
    struct Background
    {
      Background();
      Background(const std::pair<std::string, std::string>& resource_id);
      Background(const Color& color);

      std::optional<std::pair<std::string, std::string>> image;
      std::optional<Color> color;
    };

    struct Insets
    {
      int t = 0;
      int r = 0;
      int b = 0;
      int l = 0;

      Insets() = default;
      Insets(const Insets& other);
      Insets(int t, int r, int b, int l);
      /** Uniform horizontal and vertical insets. */
      Insets(int h, int v);

      Insets operator*(int amount) const;
      Insets& operator=(const Insets& other) = default;
      bool operator==(const Insets& other) const;

      void apply_to(Dimension& dimension);
      Dimension remove_from(const Dimension& dimension);
      Rect apply_to(const Rect& rect) const;
    };

    Style();
    Style(const Style& other);

    void operator=(const Style& other);

    /** Visual */
    std::optional<Background> background = std::nullopt;
    std::optional<Insets> padding = std::nullopt;

    /** Own sizing. Absent means fill remaining space. width/height are inner (content) dimensions. */
    std::optional<int> width;
    std::optional<int> height;

    /** Outer (total) dimensions: content size plus horizontal/vertical padding. */
    int total_width() const;
    int total_height() const;

    /** Positioning. Absent defaults to FLOW. */
    std::optional<PositionMode> position;
    std::optional<int> top;
    std::optional<int> left;

    /** Direction this mode uses to lay out its children. Absent defaults to COLUMN. */
    std::optional<LayoutDirection> direction;

    /** When true, excluded from hit-testing and rendering. Layout space is preserved. */
    std::optional<bool> hidden;

    /** Hover variant - merged on top of base style when :hovered is true in state. */
    std::unique_ptr<Style> hover = nullptr;
  };

  /**
   * Merge properties from a style variant map into a base style in-place.
   * Only set fields in variant override corresponding fields in out.
   */
  void apply_style_variant(Style& out, const Style& variant);

  /**
   * Resolve the effective style from a component's declared style and current
   * state. Applies the base style first, then overlays the hover variant if
   * :hovered is truthy in state.
   */
  Style resolve_style(const std::optional<Style>& style, const Lisple::sptr_rtval& state);

} // namespace Pixils::UI

#endif /* PIXILS__UI__STYLE_H */
