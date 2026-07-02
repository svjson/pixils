
#ifndef PIXILS__UI__STYLE_H
#define PIXILS__UI__STYLE_H

#include <pixils/geom.h>
#include <pixils/text.h>
#include <pixils/ui/cursor.h>
#include <pixils/ui/interaction.h>

#include <SDL3/SDL_render.h>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace Roo
{
  struct Value;
  using sptr_val = std::shared_ptr<Value>;
} // namespace Roo

namespace Pixils::UI
{
  /**
   * How a child is positioned within its parent's layout.
   */
  enum class PositionMode
  {
    FLOW,     // participates in parent flow (default)
    ABSOLUTE, // positioned at :top/:left relative to parent content rect
  };

  /**
   * Direction in which a mode lays out its children.
   */
  enum class LayoutDirection
  {
    COLUMN, // top to bottom (default)
    ROW,    // left to right
  };

  struct Style
  {
    enum class BoxSizing : uint8_t
    {
      BORDER_BOX,
      CONTENT_BOX,
    };

    enum class Visibility : uint8_t
    {
      VISIBLE,
      HIDDEN,
      NONE,
    };

    struct Background
    {
      enum class Fit : uint8_t
      {
        NONE,
        CONTAIN,
        COVER,
        FILL,
      };

      enum class Align : uint8_t
      {
        START,
        CENTER,
        END,
      };

      Background();
      Background(const std::pair<std::string, std::string>& resource_id);
      Background(const Color& color);

      std::optional<std::pair<std::string, std::string>> image;
      std::optional<Color> color;
      std::optional<Rect> source;
      std::optional<Fit> fit;
      std::optional<Align> align_x;
      std::optional<Align> align_y;
      std::optional<Point> offset;
      std::optional<float> opacity = std::nullopt;
      std::optional<bool> repeat_x;
      std::optional<bool> repeat_y;
    };

    struct Image
    {
      std::optional<float> opacity = std::nullopt;
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
      /**
       * Uniform horizontal and vertical insets.
       */
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

    struct CornerRadius
    {
      int tl = 0;
      int tr = 0;
      int br = 0;
      int bl = 0;

      CornerRadius() = default;
      CornerRadius(int amount);
      CornerRadius(int tl, int tr, int br, int bl);

      bool operator==(const CornerRadius& other) const;
      bool has_radius() const;
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
      enum class AlignItems : uint8_t
      {
        START,
        CENTER,
        END,
      };

      enum class GapMode : uint8_t
      {
        NONE,
        FIXED,
        SPACE_BETWEEN,
      };

      enum class Wrap : uint8_t
      {
        NONE,
        LINE,
      };

      struct Gap
      {
        std::optional<GapMode> mode = std::nullopt;
        std::optional<int> size = std::nullopt;
      };

      std::optional<LayoutDirection> direction = std::nullopt;
      std::optional<AlignItems> align_items = std::nullopt;
      std::optional<Gap> gap = std::nullopt;
      std::optional<Wrap> wrap = std::nullopt;
      std::optional<int> line_gap = std::nullopt;
    };

    struct Text
    {
      enum class Wrap : uint8_t
      {
        WORD,
        NONE,
      };

      struct MarkedStyle
      {
        std::optional<bool> enabled = std::nullopt;
        std::optional<char> marker = std::nullopt;
        std::optional<bool> use_font_color = std::nullopt;
        std::optional<Color> color = std::nullopt;
        std::optional<std::string> font = std::nullopt;
        std::optional<float> scale = std::nullopt;
        std::optional<std::vector<Pixils::Text::FontStyle>> font_styles = std::nullopt;
        std::optional<std::vector<Pixils::Text::Shadow>> shadows = std::nullopt;
      };

      /** Prevents inherited tint and renders bitmap/native font colors as-is. */
      bool use_font_color = false;
      std::optional<Color> color = std::nullopt;
      std::optional<std::string> font = std::nullopt;
      std::optional<float> scale = std::nullopt;
      std::optional<std::vector<Pixils::Text::FontStyle>> font_styles = std::nullopt;
      std::optional<std::vector<Pixils::Text::Shadow>> shadows = std::nullopt;
      std::optional<Pixils::Text::Alignment> align = std::nullopt;
      std::optional<Wrap> wrap = std::nullopt;
      std::optional<MarkedStyle> marked_style = std::nullopt;
    };

    struct Size
    {
      enum class Mode : uint8_t
      {
        AUTO,
        FILL,
        SHRINK,
        FIXED,
      };

      Mode mode = Mode::AUTO;
      std::optional<int> value = std::nullopt;

      Size() = default;
      Size(int fixed_value);
      Size(Mode mode);

      bool is_auto() const;
      bool is_fill() const;
      bool is_shrink() const;
      bool is_fixed() const;
      int fixed_value_or(int fallback = 0) const;
    };

    Style();
    Style(const Style& other);
    Style(Style&& other) noexcept = default;

#ifdef PIXILS_ENABLE_BENCHMARK_COUNTERS
    static void* operator new(std::size_t size);
    static void operator delete(void* ptr) noexcept;
    static void operator delete(void* ptr, std::size_t size) noexcept;
#endif

    void operator=(const Style& other);
    Style& operator=(Style&& other) noexcept = default;

    /** Visual */
    std::optional<Background> background = std::nullopt;
    std::optional<Image> image = std::nullopt;
    std::optional<Insets> margin = std::nullopt;
    std::optional<Insets> padding = std::nullopt;
    std::optional<CornerRadius> corner_radius = std::nullopt;
    std::optional<BorderStyle> border = std::nullopt;
    std::optional<Text> text = std::nullopt;
    std::optional<BoxSizing> box_sizing = std::nullopt;
    std::optional<int> scale = std::nullopt;
    std::optional<float> opacity = std::nullopt;

    /**
     * Preferred size. For fixed numeric values, interpretation depends on
     * box_sizing: border-box by default, or content-box when explicitly set.
     * Absent means fill remaining space.
     */
    std::optional<Size> width;
    std::optional<Size> height;
    std::optional<int> min_width;
    std::optional<int> min_height;
    std::optional<int> max_width;
    std::optional<int> max_height;

    /** Outer flow dimensions: view box plus margins. */
    int total_width() const;
    int total_height() const;

    /**
     * Content rect within the given bounds: inset by border thickness then by
     * padding. This is the area available to children and the render hook.
     */
    Rect content_rect(const Rect& bounds) const;

    /**
     * Positioning. Absent defaults to FLOW.
     */
    std::optional<PositionMode> position;
    std::optional<int> top;
    std::optional<int> left;

    /**
     * Layout policy for this mode's children.
     */
    std::optional<Layout> layout;

    /**
     * Visibility controls rendering, hit-testing, and layout participation.
     * HIDDEN keeps layout space but skips rendering and interaction.
     * NONE skips layout, rendering, and interaction.
     */
    std::optional<Visibility> visibility;

    /**
     * When false, excluded from hit-testing while still rendering normally.
     */
    std::optional<bool> hit_test;

    /**
     * When true, descendant rendering and hit testing are clipped to content_rect.
     */
    std::optional<bool> clip;

    /**
     * OS cursor used while this view, or a descendant without an override, is hovered.
     */
    std::optional<CursorSpec> cursor;

    /** Hover variant - merged on top of base style when the cursor is within bounds. */
    std::unique_ptr<Style> hover = nullptr;
    /** Focus-within variant - merged when this view or any descendant owns focus. */
    std::unique_ptr<Style> focus_within = nullptr;
    /** Focus variant - merged when this exact view owns focus. */
    std::unique_ptr<Style> focus = nullptr;
  };

  /**
   * Merge properties from a style variant map into a base style in-place.
   * Only set fields in variant override corresponding fields in out.
   */
  void apply_style_variant(Style& out, const Style& variant);

  /**
   * Resolve the effective style from a component's declared style and current
   * state. Applies any inherited cascading properties first, then the base
   * style, then overlays hover/focus variants from broadest to narrowest.
   */
  Style resolve_style(const std::optional<Style>& style,
                      const Style* inherited_style,
                      const Roo::sptr_val& state,
                      const InteractionState& interaction = {},
                      const Style* default_style = nullptr);

  inline Style resolve_style(const std::optional<Style>& style,
                             const Roo::sptr_val& state,
                             const InteractionState& interaction = {})
  {
    return resolve_style(style, nullptr, state, interaction);
  }

} // namespace Pixils::UI

#endif /* PIXILS__UI__STYLE_H */
