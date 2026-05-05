
#ifndef PIXILS__UI__STYLE_H
#define PIXILS__UI__STYLE_H

#include <pixils/geom.h>
#include <pixils/ui/interaction.h>

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

    struct Trim
    {
      int start = 0;
      int end = 0;

      Trim() = default;
      Trim(int amount);
      Trim(int start, int end);

      bool operator==(const Trim& other) const;
    };

    enum class LineStyle : uint8_t
    {
      SOLID,
      BEVEL,
    };

    struct Border
    {
      virtual ~Border() = default;

      std::optional<int> thickness = std::nullopt;
      std::optional<LineStyle> line_style = std::nullopt;
      std::optional<Color> color = std::nullopt;
      std::optional<Trim> trim = std::nullopt;
    };

    /**
     * @brief Contains the full border configuration of a Style unit
     */
    struct BorderStyle : public Border
    {
      std::optional<Border> t = std::nullopt;
      std::optional<Border> r = std::nullopt;
      std::optional<Border> b = std::nullopt;
      std::optional<Border> l = std::nullopt;

      /**
       * Effective thickness for each side: per-side value if set, else base.
       */
      int top_thickness() const;
      int right_thickness() const;
      int bottom_thickness() const;
      int left_thickness() const;

      /**
       * Effective color for each side: per-side value if set, else base.
       */
      std::optional<Color> top_color() const;
      std::optional<Color> right_color() const;
      std::optional<Color> bottom_color() const;
      std::optional<Color> left_color() const;

      /**
       * Effective trim for each side: per-side value if set, else base, else zero.
       */
      Trim top_trim() const;
      Trim right_trim() const;
      Trim bottom_trim() const;
      Trim left_trim() const;

      /**
       * Effective line style for each side: per-side value if set, else base, else solid.
       */
      LineStyle top_line_style() const;
      LineStyle right_line_style() const;
      LineStyle bottom_line_style() const;
      LineStyle left_line_style() const;

      /**
       * Inset a rect by the effective border thickness on each side.
       */
      Rect apply_to(const Rect& bounds) const;
    };

    struct Layout
    {
      enum class GapMode : uint8_t
      {
        NONE,
        FIXED,
        SPACE_BETWEEN,
      };

      struct Gap
      {
        std::optional<GapMode> mode = std::nullopt;
        std::optional<int> size = std::nullopt;
      };

      std::optional<LayoutDirection> direction = std::nullopt;
      std::optional<Gap> gap = std::nullopt;
    };

    struct Text
    {
      std::optional<Color> color = std::nullopt;
      std::optional<std::string> font = std::nullopt;
      std::optional<int> scale = std::nullopt;
    };

    Style();
    Style(const Style& other);

    void operator=(const Style& other);

    /** Visual */
    std::optional<Background> background = std::nullopt;
    std::optional<Insets> margin = std::nullopt;
    std::optional<Insets> padding = std::nullopt;
    std::optional<BorderStyle> border = std::nullopt;
    std::optional<Text> text = std::nullopt;

    /**
     * Content surface size. Absent means fill remaining space. When
     * set, this provides the preferred width/height for the inner
     * (content) dimensions of an associated view.
     */
    std::optional<int> width;
    std::optional<int> height;

    /** Outer (total) dimensions: content size plus padding and border. */
    int total_width() const;
    int total_height() const;

    /**
     * Content rect within the given bounds: inset by border thickness then by
     * padding. This is the area available to children and the render hook.
     */
    Rect content_rect(const Rect& bounds) const;

    /** Positioning. Absent defaults to FLOW. */
    std::optional<PositionMode> position;
    std::optional<int> top;
    std::optional<int> left;

    /** Layout policy for this mode's children. */
    std::optional<Layout> layout;

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
  Style resolve_style(const std::optional<Style>& style,
                      const Lisple::sptr_rtval& state,
                      const InteractionState& interaction = {});

} // namespace Pixils::UI

#endif /* PIXILS__UI__STYLE_H */
