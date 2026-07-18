
#ifndef PIXILS__TEXT_H
#define PIXILS__TEXT_H

#include <pixils/geom.h>

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

typedef struct SDL_Texture SDL_Texture;

namespace Pixils
{
  struct RenderContext;

  namespace Style
  {
    class Style;
  }

  namespace Text
  {
    class Renderer;

    enum class FontStyle : uint8_t
    {
      UNDERLINE,
    };

    struct Scale
    {
      float x = 1.0f;
      float y = 1.0f;

      Scale() = default;
      Scale(float uniform);
      Scale(float x, float y);
    };

    struct UnderlineMetrics
    {
      int offset = 0;
      int thickness = 1;
    };

    struct FontDefinition
    {
      int baseline = 0;
      std::optional<UnderlineMetrics> underline = std::nullopt;
    };

    /*!
     * @brief Describes a text shadow - its offset and color
     */
    struct Shadow
    {
      /*! @brief The offset of the shadow, in pixels */
      Point offset;
      /*! @brief The shadow color */
      Color color;
      /*! @brief Copy constructor */
      Shadow(const Shadow& other);
      /*!
       * @brief Constructs a shadow, represented by its @ref
       * FrontierWorlds::Color and offset @ref FrontierWorlds::Coordinate
       */
      Shadow(const Point& offset, const Color& color);

      /*! @brief Default assignment operator */
      Shadow& operator=(const Shadow& other) = default;
      /*!
       * @brief Comparison operator for comparing the value of another
       * shadow instance to this one.
       */
      bool operator==(const Shadow& other) const;
    };

    struct InlineTextStyleSpec
    {
      bool enabled = false;
      char marker = '@';
      std::optional<std::string> font_key = std::nullopt;
      std::optional<Scale> scale = std::nullopt;
      bool use_font_color = false;
      std::optional<Color> color = std::nullopt;
      std::optional<std::vector<FontStyle>> font_styles = std::nullopt;
      std::optional<std::vector<Shadow>> shadows = std::nullopt;
    };

    struct InlineTextRenderOp
    {
      bool enabled = false;
      char marker = '@';
      Renderer* renderer = nullptr;
      Renderer* tint_renderer = nullptr;
      SDL_Color color = {0xff, 0xff, 0xff, 0xff};
      const FontDefinition* font_definition = nullptr;
      std::vector<FontStyle> font_styles;
      std::vector<Shadow> shadows;
    };

    /*!
     * @brief Prepared text rendering configuration resolved from font key,
     * scale and optional tint color.
     */
    struct TextRenderOp
    {
      Renderer* renderer;
      Renderer* tint_renderer;
      SDL_Color color;
      const FontDefinition* font_definition = nullptr;
      std::vector<FontStyle> font_styles;
      std::vector<Shadow> shadows;
      std::optional<InlineTextRenderOp> inline_style = std::nullopt;
    };

    struct LayoutSegment
    {
      std::string text;
      bool use_inline_style = false;
      int width = 0;
    };

    struct LayoutLine
    {
      std::string text;
      std::vector<LayoutSegment> segments;
      int width = 0;
    };

    struct Layout
    {
      std::vector<LayoutLine> lines;
      Dimension size = {0, 0};
    };

    enum class WrapMode : uint8_t
    {
      NONE,
      WORD,
    };

    /*!
     * @brief Wrapper class that keeps track of font letters within a sprite map
     * texture containing the font graphics
     */
    class FontMap
    {
      /*!
       * @brief Maps a character to rect in the sprite map
       */
      std::map<char32_t, SDL_Rect> map;

     public:
      /*!
       * @brief Constructs a new FontMap instance
       */
      FontMap(const std::map<char32_t, SDL_Rect>& map);

      /*!
       * @brief Queries the FontMap if a definition exists for a particular
       * character.
       */
      bool has_char(char32_t chr) const;

      /*!
       * @brief Returns the rect that defines to location of a character inside
       * the sprite map, if it exists. Returns nullptr if the character is not
       * defined.
       */
      SDL_Rect* get_char_rect(char32_t chr);

      /*!
       * @brief Returns a list of all defined characters
       */
      std::vector<char32_t> keys();

      /*!
       * @brief Set or overwrite a character definition.
       */
      void set_char(char32_t chr, const SDL_Rect& rect);

      /*!
       * @brief Returns the tallest glyph height defined in this map.
       */
      int tallest_glyph_height() const;
    };

    /*!
     * @brief a small convenience class for rendering string content as
     * graphical text using an SDL_Texture* containing a sprite map of
     * a font and a FontMap describing the grahics content.
     */
    class Renderer
    {
      /*!
       * @brief The texture associated with the FontMap
       */
      SDL_Texture* font;

      /*!
       * @brief The font that describes the coordinates of individual
       * characters within the @ref font spritemap.
       */
      FontMap& font_map;

      /*!
       * @brief The alternative color to use when toggled.
       */
      SDL_Color alt_color = {0, 0, 0, 0xff};
      int spacing;
      int line_height;
      /*!
       * @brief The scale of the text to render. This can be used to render
       * text at arbitrary multiples of its original size
       */
      Scale scale = {};

     public:
      Renderer(SDL_Texture* font,
               FontMap& font_map,
               int spacing = 1,
               float scale = 1.0f,
               int line_height = 0);

      /*!
       * @brief Renders a text string at the specified location of the render
       * target, and with the specified color.
       */
      void render_text(RenderContext& rc,
                       const std::string& text,
                       int x,
                       int y,
                       const SDL_Color& color);

      /*!
       * @brief Calculates the renderered size of a string without actually rendering
       * anything. Useful for layout calculations.
       */
      SDL_Rect get_rendered_size(RenderContext& rc, const std::string& string) const;

      /*!
       * @brief Returns the height, in pixels, of the configured font.
       *
       * All characters are expected to be of the same height, so the value
       * is lifted from the first character of the font. If we are going to
       * need variable height characters, we will have to iterate through
       * all characters and find the max value for height(and ideally cache it)
       */
      int get_font_height() const;
      /*!
       * @brief Returns the configured default line height, in pixels.
       */
      int get_line_height() const;
      /*!
       * @brief Returns the rendered advance width for a single character.
       */
      int get_char_advance(char32_t chr) const;
      /*!
       * @brief Set the alternative text color of this renderer.
       */
      void set_alt_color(const SDL_Color& color);
      /*!
       * @brief Get the scale modifier of this Renderer
       */
      float get_scale() const;
      /*!
       * @brief Get the horizontal scale modifier of this Renderer
       */
      float get_scale_x() const;
      /*!
       * @brief Get the vertical scale modifier of this Renderer
       */
      float get_scale_y() const;
      /*!
       * @brief Set the scale modifier of this Renderer
       */
      void set_scale(float scale);
      /*!
       * @brief Set independent horizontal and vertical scale modifiers
       */
      void set_scale(float scale_x, float scale_y);
      /*!
       * @brief Set the scale modifier of this Renderer
       */
      void set_scale(const Scale& scale);

      /*!
       * @brief Queries the underlying FontMap and font if it supports a
       * particular character.
       */
      bool supports_char(char32_t c) const;
    };

    /*!
     * @brief Enum for text alignment
     */
    enum class Alignment : uint8_t
    {
      /*! @brief Aligns text to the left */
      LEFT = 0,
      /*! @brief Aligns text to the center */
      CENTER = 1,
      /*! @brief Aligns text to the right */
      RIGHT = 2
    };

    /*!
     * @brief Convenience wrapper around Text::Renderer that provides a cursor
     * abstraction, assisting with newlines and text flow
     *
     * The Cursor contains state in the sense that it tracks its current
     * position within the render target, and moves it forward upon
     * outputting text.
     */
    class Cursor
    {
     public:
      /*!
       * @brief Constructs a new Text::Cursor and configures it to use
       * the supplied renderer and colors.
       *
       * The default height of the underlying font will serve as
       * @ref line_height for the Cursor.
       */
      Cursor(Renderer& renderer, const SDL_Color& color, const SDL_Color& alt_color);

      /*!
       * @brief Constructs a new Text::Cursor and configures it to begin
       * text rendering at the speficifed location, and to use
       * the supplied renderer and single color for both regular and
       * highlighted text, and with a custom line height, overriding
       * the default font height.
       */
      Cursor(Renderer& renderer,
             const Point& position,
             const SDL_Color& color,
             int line_height);

      /*!
       * @brief Constructs a new Text::Cursor and configures it to use
       * the supplied renderer and single color for both regular and
       * highlighted text, and with a custom line height, overriding
       * the default font height.
       */
      Cursor(Renderer& renderer, const SDL_Color& color, int line_height);

      /*!
       * @brief Constructs a new Text::Cursor and configures it to use
       * the supplied renderers, colors and custom line height, overriding
       * any default values of the Text::Renderer.
       */
      Cursor(Renderer& renderer,
             const SDL_Color& color,
             const SDL_Color& alt_color,
             int line_height);

      /*!
       * @brief Constructs a new Text::Cursor and configures it to begin
       * text output at the specified position and to use
       * the supplied renderers, colors and custom line height, overriding
       * any default values of the Text::Renderer.
       */
      Cursor(Renderer& renderer,
             const Point& position,
             const SDL_Color& color,
             const SDL_Color& alt_color,
             int line_height);

      /*!
       * @brief Get the current position of the text cursor
       */
      const Point& get_position() const;

      /*!
       * @brief get the underlying Text::Renderer
       */
      Renderer& get_renderer();

      /*!
       * @brief Prints the specified string at the cursor location, and
       * moves the cursor
       * to the end of the printed string.
       */
      void print(RenderContext& rc, const std::string& text);
      /*!
       * @brief Prints the specified string at the cursor location in the
       * specified color, and moves the cursor to the end of the printed
       * string.
       */
      void print(RenderContext& rc, const std::string& text, const SDL_Color& color);
      /*!
       * @brief Prints the specified string at the cursor location in the
       * specified color - and additionally fills the background of the text
       * rect with a background color - and then moves the cursor to the end of the
       * printed string.
       *
       * Useful for labels and tooltips
       */
      void print(RenderContext& rc,
                 const std::string& text,
                 const SDL_Color& color,
                 const SDL_Color& background_color);
      /*!
       * @brief Prints the specified string at the cursor location, inserts a
       * "newline" at the end, by movin the cursor to the beginning of the
       * next line.
       *
       * What constitutes the "beginning of the next line" is specified by
       * @ref Cursor::line_start_x
       */
      void println(RenderContext& rc, const std::string& text);

      /*!
       * @brief Prints the specified string at the cursor location, overriding
       * the configured color, and inserts a "newline" at the end, by movin the
       * cursor to the beginning of the next line.
       *
       * What constitutes the "beginning of the next line" is specified by
       * @ref line_start_x
       */
      void println(RenderContext& rc, const std::string& text, const SDL_Color& color);

      /*!
       * @brief Set the default color for text rendering
       */
      void set_color(const SDL_Color& color);

      /*!
       * @brief Set the alignment of the cursor.
       *
       * Using Alignment::CENTER will place the horizontal center
       * of the outputted text at the current X value of the Cursor.
       */
      void set_alignment(Alignment alignment);

      /*!
       * @brief Set the line height of the cursor, which effectively is how many
       * pixels to move vertically upon "newline":
       */
      void set_line_height(int line_height);

      /*!
       * @brief Adds a Shadow specification, causing any rendered text to
       * be rendered in the Shadow color and with the Shadow offset before
       * rendering the actual text.
       */
      void add_shadow(const Shadow& shadow);
      /*!
       * @brief Moves the cursor to a position within the render target. This
       * also sets the @ref line_start_x value to the the value of x.
       */
      void move_to(int x, int y);

      /*!
       * @brief Moves the cursor to a position within the render target. This
       * also sets the @ref line_start_x value to the the value of x
       * of the supplied @ref FrontierWorlds::Coordinate.
       */
      void move_to(const Point& coordinate);

      /*!
       * @brief Get the line height of the cursor, which effectively is how many
       * pixels to move vertically upon "newline":
       */
      int get_line_height() const;

      /*!
       * @brief Get the configured text alignment of the Cursor.
       */
      Alignment get_alignment() const;

      /*!
       * @brief Calculates the rendered size of a text string, without
       * actually rendering it.
       *
       * Essentially the same as Text::Renderer::get_rendered_size, but this
       * function also takes the cursor position and @ref Cursor::alignment
       * into consideration
       */
      SDL_Rect get_rendered_rect(RenderContext& rc, const std::string& text);

     private:
      /*!
       * @brief Internal function used to perform text rendering.
       *
       * The public functions for printing text invariably use this.
       */
      void render_text(RenderContext& rc, const std::string& text, const SDL_Color& color);

      /*!
       * @brief The underlying @ref Text::Renderer that performs any actual
       * rendering.
       */
      Renderer& renderer;

      /*!
       * @brief The current position of the Cursor.
       */
      Point position;

      /*!
       * @brief The text alignment of the Cursor.
       *
       * Alignment::LEFT will output text to the right of the current
       * text position
       *
       * Alignment::CENTER will output text so that the horizontal
       * center of the rendered text is located at the current cursor position.
       *
       * Alignment::RIGHT will output text so that the right edge
       * of the rendered text is located at the current cursor position.
       */
      Alignment alignment = Alignment::LEFT;

      /*!
       * @brief The default color for rendered text
       */
      SDL_Color color;
      /*!
       * @brief The default alternative color for render text, that is used
       * when a highlight toggle symbol is encountered.
       */
      SDL_Color alt_color;
      /*!
       * @brief Effectively specifies how many pixels to move vertically
       * upon newline.
       */
      int line_height;
      /*!
       * @brief Specifices the X position of where the Cursor considers the
       * start of a line is.
       *
       * This is updated every time the Cursor position is manually set,
       * using @ref move_to
       */
      int line_start_x;

      /*!
       * @brief Configured text shadows for this Cursor. Renders them
       * before the main text at the specified offset of each Shadow.
       */
      std::vector<Shadow> shadows;
    };

    /*!
     * @brief Resolves a font/scale/color combination into a reusable text render op.
     */
    std::optional<TextRenderOp> make_text_render_op(
      RenderContext& rc,
      const std::string& font_key = "font/console",
      Scale scale = {},
      const std::optional<Color>& color = std::nullopt,
      const std::vector<FontStyle>& font_styles = {},
      const std::vector<Shadow>& shadows = {},
      const std::optional<InlineTextStyleSpec>& inline_style = std::nullopt);

    /*!
     * @brief Calculates the rendered size of a string using a prepared text render op.
     */
    SDL_Rect calculate_rendered_size(RenderContext& rc,
                                     const TextRenderOp& op,
                                     const std::string& string);

    Layout layout_text(RenderContext& rc,
                       const TextRenderOp& op,
                       const std::string& text,
                       WrapMode wrap_mode = WrapMode::NONE,
                       std::optional<int> max_width = std::nullopt);

    /*!
     * @brief Renders a string using a prepared text render op.
     */
    void render_text(RenderContext& rc,
                     const TextRenderOp& op,
                     const std::string& text,
                     int x,
                     int y);

    /*!
     * @brief Renders an already laid out text line without re-parsing inline markers.
     */
    void render_layout_line(RenderContext& rc,
                            const TextRenderOp& op,
                            const LayoutLine& line,
                            int x,
                            int y);

  } // namespace Text
} // namespace Pixils

#endif
