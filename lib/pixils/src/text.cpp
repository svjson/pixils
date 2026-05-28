
#include <pixils/context.h>
#include <pixils/benchmark/counters.h>
#include <pixils/font_registry.h>
#include <pixils/geom.h>
#include <pixils/text.h>

#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <stddef.h>

namespace Pixils
{
  namespace Text
  {
    Scale::Scale(int uniform)
      : x(uniform)
      , y(uniform)
    {
    }

    Scale::Scale(int x, int y)
      : x(x)
      , y(y)
    {
    }

    namespace
    {
      struct StyledSegment
      {
        std::string text;
        bool use_inline_style = false;
      };

      bool is_wrap_whitespace(char c)
      {
        return c == ' ' || c == '\t';
      }

      std::vector<StyledSegment> split_inline_segments(const std::string& text,
                                                       const TextRenderOp& op)
      {
        std::vector<StyledSegment> segments;
        const bool inline_enabled = op.inline_style && op.inline_style->enabled;
        const char marker = inline_enabled ? op.inline_style->marker : '\0';
        bool use_inline_style = false;
        std::string current;

        auto flush = [&]()
        {
          if (current.empty()) return;
          segments.push_back({current, use_inline_style});
          current.clear();
        };

        for (size_t i = 0; i < text.size(); i++)
        {
          char c = text.at(i);
          if (inline_enabled && c == marker)
          {
            if (i + 1 < text.size() && text.at(i + 1) == marker)
            {
              current.push_back(marker);
              i++;
              continue;
            }

            flush();
            use_inline_style = !use_inline_style;
            continue;
          }

          current.push_back(c);
        }

        flush();
        return segments;
      }

      std::vector<StyledSegment> split_marker_segments(const std::string& text,
                                                       char marker = '@')
      {
        std::vector<StyledSegment> segments;
        bool use_alt_style = false;
        std::string current;

        auto flush = [&]()
        {
          if (current.empty()) return;
          segments.push_back({current, use_alt_style});
          current.clear();
        };

        for (size_t i = 0; i < text.size(); i++)
        {
          char c = text.at(i);
          if (c == marker)
          {
            if (i + 1 < text.size() && text.at(i + 1) == marker)
            {
              current.push_back(marker);
              i++;
              continue;
            }

            flush();
            use_alt_style = !use_alt_style;
            continue;
          }

          current.push_back(c);
        }

        flush();
        return segments;
      }

      SDL_Rect marker_aware_rendered_size(RenderContext& rc,
                                          Renderer& renderer,
                                          const std::string& text)
      {
        SDL_Rect rect{0, 0, 0, renderer.get_line_height() * renderer.get_scale_y()};
        auto segments = split_marker_segments(text);
        for (const auto& segment : segments)
        {
          rect.w += renderer.get_rendered_size(rc, segment.text).w;
        }
        return rect;
      }

      const Renderer& select_renderer(const TextRenderOp& op, bool use_inline_style)
      {
        if (use_inline_style && op.inline_style && op.inline_style->enabled &&
            op.inline_style->renderer)
        {
          return *op.inline_style->renderer;
        }

        return *op.renderer;
      }

      Renderer& select_renderer(TextRenderOp& op, bool use_inline_style)
      {
        if (use_inline_style && op.inline_style && op.inline_style->enabled &&
            op.inline_style->renderer)
        {
          return *op.inline_style->renderer;
        }

        return *op.renderer;
      }

      Renderer& select_tint_renderer(TextRenderOp& op, bool use_inline_style)
      {
        if (use_inline_style && op.inline_style && op.inline_style->enabled &&
            op.inline_style->tint_renderer)
        {
          return *op.inline_style->tint_renderer;
        }

        return *op.tint_renderer;
      }

      const FontDefinition* select_font_definition(const TextRenderOp& op,
                                                   bool use_inline_style)
      {
        if (use_inline_style && op.inline_style && op.inline_style->enabled)
        {
          return op.inline_style->font_definition;
        }

        return op.font_definition;
      }

      const SDL_Color& select_color(const TextRenderOp& op, bool use_inline_style)
      {
        if (use_inline_style && op.inline_style && op.inline_style->enabled)
        {
          return op.inline_style->color;
        }

        return op.color;
      }

      const std::vector<FontStyle>& select_font_styles(const TextRenderOp& op,
                                                       bool use_inline_style)
      {
        if (use_inline_style && op.inline_style && op.inline_style->enabled)
        {
          return op.inline_style->font_styles;
        }

        return op.font_styles;
      }

      const std::vector<Shadow>& select_shadows(const TextRenderOp& op,
                                                bool use_inline_style)
      {
        if (use_inline_style && op.inline_style && op.inline_style->enabled)
        {
          return op.inline_style->shadows;
        }

        return op.shadows;
      }

      int op_line_height(const TextRenderOp& op)
      {
        auto style_height = [](const Renderer& renderer,
                               const FontDefinition* font_definition,
                               const std::vector<FontStyle>& font_styles)
        {
          int height = renderer.get_line_height() * renderer.get_scale_y();
          for (auto style : font_styles)
          {
            if (style == FontStyle::UNDERLINE && font_definition &&
                font_definition->underline)
            {
              height =
                std::max(height,
                         (font_definition->baseline + font_definition->underline->offset +
                          font_definition->underline->thickness) *
                           renderer.get_scale_y());
            }
          }
          return height;
        };

        int line_height = style_height(*op.renderer, op.font_definition, op.font_styles);
        if (op.inline_style && op.inline_style->enabled && op.inline_style->renderer)
        {
          line_height = std::max(line_height,
                                 style_height(*op.inline_style->renderer,
                                              op.inline_style->font_definition,
                                              op.inline_style->font_styles));
        }
        return line_height;
      }

      int underline_top(const Renderer& renderer,
                        const FontDefinition* font_definition,
                        const std::vector<FontStyle>& font_styles,
                        int y)
      {
        if (!font_definition)
          return y + renderer.get_line_height() * renderer.get_scale_y() - 1;

        for (auto style : font_styles)
        {
          if (style == FontStyle::UNDERLINE && font_definition->underline)
          {
            return y + (font_definition->baseline + font_definition->underline->offset) *
                         renderer.get_scale_y();
          }
        }

        return y + renderer.get_line_height() * renderer.get_scale_y() - 1;
      }

      int underline_thickness(const Renderer& renderer,
                              const FontDefinition* font_definition,
                              const std::vector<FontStyle>& font_styles)
      {
        for (auto style : font_styles)
        {
          if (style == FontStyle::UNDERLINE && font_definition && font_definition->underline)
          {
            return std::max(1,
                            font_definition->underline->thickness *
                              renderer.get_scale_y());
          }
        }

        return 0;
      }

      void render_underlines(RenderContext& rc,
                             const TextRenderOp& op,
                             bool use_inline_style,
                             int x,
                             int y,
                             int width,
                             const SDL_Color& color)
      {
        if (width <= 0) return;

        const auto& font_styles = select_font_styles(op, use_inline_style);
        const Renderer& renderer = select_renderer(op, use_inline_style);
        const FontDefinition* font_definition = select_font_definition(op, use_inline_style);
        int thickness = underline_thickness(renderer, font_definition, font_styles);
        if (thickness <= 0) return;

        SDL_Rect rect{x,
                      underline_top(renderer, font_definition, font_styles, y),
                      width,
                      thickness};
        SDL_SetRenderDrawColor(rc.renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(rc.renderer, &rect);
      }

      int rendered_width_for_segment(RenderContext& rc,
                                     const TextRenderOp& op,
                                     const StyledSegment& segment)
      {
        return select_renderer(op, segment.use_inline_style)
          .get_rendered_size(rc, segment.text)
          .w;
      }

      std::vector<std::string> split_paragraphs(const std::string& text)
      {
        std::vector<std::string> paragraphs;
        std::string current;

        for (size_t i = 0; i < text.size(); i++)
        {
          char c = text[i];
          if (c == '\r')
          {
            if (i + 1 < text.size() && text[i + 1] == '\n')
            {
              i++;
            }
            paragraphs.push_back(current);
            current.clear();
            continue;
          }

          if (c == '\n')
          {
            paragraphs.push_back(current);
            current.clear();
            continue;
          }

          current.push_back(c);
        }

        paragraphs.push_back(current);
        return paragraphs;
      }
    } // namespace

    /* FontMap */
    FontMap::FontMap(const std::map<char32_t, SDL_Rect>& map)
      : map(map)
    {
    }

    bool FontMap::has_char(char32_t chr) const
    {
      return map.count(chr);
    }

    SDL_Rect* FontMap::get_char_rect(char32_t chr)
    {
      if (map.count(chr))
      {
        return &map.at(chr);
      }
      return nullptr;
    }

    void FontMap::set_char(char32_t chr, const SDL_Rect& rect)
    {
      if (map.count(chr))
      {
        map.at(chr) = rect;
      }
      else
      {
        map.emplace(chr, rect);
      }
    }

    int FontMap::tallest_glyph_height() const
    {
      int height = 0;
      for (const auto& [_, rect] : map)
      {
        height = std::max(height, rect.h);
      }
      return height;
    }

    std::vector<char32_t> FontMap::keys()
    {
      std::vector<char32_t> chars;
      for (auto [k, _] : map)
      {
        chars.push_back(k);
      }
      return chars;
    }

    /**
     * @brief Renderer - Renders string content as graphical text
     */
    Renderer::Renderer(SDL_Texture* font,
                       FontMap& font_map,
                       int spacing,
                       int scale,
                       int line_height)
      : font(font)
      , font_map(font_map)
      , spacing(spacing)
      , line_height(std::max(line_height, font_map.tallest_glyph_height()))
      , scale(scale)
    {
    }

    void Renderer::set_alt_color(const SDL_Color& color)
    {
      this->alt_color = color;
    }

    bool Renderer::supports_char(char32_t c) const
    {
      return font_map.has_char(c) > 0;
    }

    void Renderer::render_text(RenderContext& rc,
                               const std::string& text,
                               int x,
                               int y,
                               const SDL_Color& color)
    {
      SDL_Rect cursor;
      cursor.x = x;
      cursor.y = y;

      SDL_SetTextureColorMod(font, color.r, color.g, color.b);
      SDL_SetTextureAlphaMod(font, color.a);

      for (size_t i = 0; i < text.size(); i++)
      {
        char c = text.at(i);
        if (!font_map.has_char(c))
        {
          if (c == 10)
            c = ' ';
          else
            continue;
        }

        const SDL_Rect& char_rect = *font_map.get_char_rect(c);
        cursor.w = char_rect.w * scale.x;
        cursor.h = char_rect.h * scale.y;

        PIXILS_BENCHMARK_COUNT(text_renderer_glyphs_rendered);
        PIXILS_BENCHMARK_COUNT(render_copy_calls);
        SDL_RenderCopy(rc.renderer, font, &char_rect, &cursor);
        cursor.x += cursor.w + (spacing * scale.x);
      }
    }

    SDL_Rect Renderer::get_rendered_size(RenderContext&, const std::string& string) const
    {
      PIXILS_BENCHMARK_COUNT(text_renderer_size_calls);
      SDL_Rect rect{0, 0, 0, 0};

      for (size_t i = 0; i < string.size(); i++)
      {
        char32_t c = string.at(i);
        if (!font_map.has_char(c)) c = ' ';
        if (font_map.has_char(c))
        {
          const SDL_Rect& char_rect = *font_map.get_char_rect(c);
          rect.w += char_rect.w + spacing;
          PIXILS_BENCHMARK_COUNT(text_renderer_glyphs_measured);
        }
      }
      rect.h = line_height;
      rect.w *= scale.x;
      rect.h *= scale.y;

      return rect;
    }

    void Renderer::set_scale(int scale)
    {
      set_scale(scale, scale);
    }

    void Renderer::set_scale(int scale_x, int scale_y)
    {
      this->scale = Scale(std::max(1, scale_x), std::max(1, scale_y));
    }

    void Renderer::set_scale(const Scale& scale)
    {
      set_scale(scale.x, scale.y);
    }

    int Renderer::get_scale() const
    {
      return this->scale.x;
    }

    int Renderer::get_scale_x() const
    {
      return this->scale.x;
    }

    int Renderer::get_scale_y() const
    {
      return this->scale.y;
    }

    int Renderer::get_font_height() const
    {
      return get_line_height();
    }

    int Renderer::get_line_height() const
    {
      return line_height;
    }

    int Renderer::get_char_advance(char32_t chr) const
    {
      char32_t c = font_map.has_char(chr) ? chr : ' ';
      if (!font_map.has_char(c)) return 0;
      const SDL_Rect& char_rect = *font_map.get_char_rect(c);
      return (char_rect.w + spacing) * scale.x;
    }

    std::optional<TextRenderOp> make_text_render_op(
      RenderContext& rc,
      const std::string& font_key,
      Scale scale,
      const std::optional<Color>& color,
      const std::vector<FontStyle>& font_styles,
      const std::vector<Shadow>& shadows,
      const std::optional<InlineTextStyleSpec>& inline_style)
    {
      if (!rc.font_registry)
      {
        PIXILS_BENCHMARK_COUNT(text_render_op_failures);
        return std::nullopt;
      }

      BitmapFont* font = rc.font_registry->get_font(font_key);
      if (!font)
      {
        PIXILS_BENCHMARK_COUNT(text_render_op_failures);
        return std::nullopt;
      }

      font->renderer.set_scale(scale);
      font->tint_renderer.set_scale(scale);

      TextRenderOp op{
        .renderer = color ? &font->tint_renderer : &font->renderer,
        .tint_renderer = &font->tint_renderer,
        .color = color ? color->to_SDL_Color() : SDL_Color{0xff, 0xff, 0xff, 0xff},
        .font_definition = &font->definition,
        .font_styles = font_styles,
        .shadows = shadows};

      if (inline_style && inline_style->enabled)
      {
        std::string inline_font_key = inline_style->font_key.value_or(font_key);
        BitmapFont* inline_font = rc.font_registry->get_font(inline_font_key);
        if (!inline_font)
        {
          PIXILS_BENCHMARK_COUNT(text_render_op_failures);
          return std::nullopt;
        }

        Scale inline_scale = inline_style->scale.value_or(scale);
        inline_font->renderer.set_scale(inline_scale);
        inline_font->tint_renderer.set_scale(inline_scale);

        bool use_font_color = inline_style->use_font_color;
        std::optional<Color> inline_color =
          use_font_color ? std::nullopt : inline_style->color;
        const bool inherit_tint_from_parent =
          !inline_color && op.renderer == op.tint_renderer;
        Renderer* inline_renderer = (inline_color || inherit_tint_from_parent)
                                      ? &inline_font->tint_renderer
                                      : &inline_font->renderer;

        op.inline_style = InlineTextRenderOp{
          .enabled = true,
          .marker = inline_style->marker,
          .renderer = inline_renderer,
          .tint_renderer = &inline_font->tint_renderer,
          .color = inline_color ? inline_color->to_SDL_Color() : op.color,
          .font_definition = &inline_font->definition,
          .font_styles = inline_style->font_styles.value_or(font_styles),
          .shadows = inline_style->shadows.value_or(shadows),
        };
      }

      PIXILS_BENCHMARK_COUNT(text_render_op_creations);
      return op;
    }

    SDL_Rect calculate_line_rendered_size(RenderContext& rc,
                                          const TextRenderOp& op,
                                          const std::string& string)
    {
      PIXILS_BENCHMARK_COUNT(text_line_measure_calls);
      SDL_Rect rect{0, 0, 0, op_line_height(op)};
      auto segments = split_inline_segments(string, op);
      for (const auto& segment : segments)
      {
        rect.w += rendered_width_for_segment(rc, op, segment);
      }
      return rect;
    }

    SDL_Rect calculate_rendered_size(RenderContext& rc,
                                     const TextRenderOp& op,
                                     const std::string& string)
    {
      PIXILS_BENCHMARK_COUNT(text_measure_calls);
      PIXILS_BENCHMARK_TIME_BLOCK(text_measure_time_ns);
      SDL_Rect rect{0, 0, 0, 0};
      const int line_height = op_line_height(op);

      for (const auto& line : split_paragraphs(string))
      {
        SDL_Rect line_rect = calculate_line_rendered_size(rc, op, line);
        rect.w = std::max(rect.w, line_rect.w);
        rect.h += line_height;
      }

      if (rect.h == 0)
      {
        rect.h = line_height;
      }

      return rect;
    }

    Layout layout_text(RenderContext& rc,
                       const TextRenderOp& op,
                       const std::string& text,
                       WrapMode wrap_mode,
                       std::optional<int> max_width)
    {
      PIXILS_BENCHMARK_COUNT(text_layout_calls);
      PIXILS_BENCHMARK_TIME_BLOCK(text_layout_time_ns);
      Layout layout;
      const int line_height = op_line_height(op);
      const bool should_wrap = wrap_mode == WrapMode::WORD && max_width && *max_width > 0;

      auto measure_width = [&](const std::string& line)
      {
        return calculate_line_rendered_size(rc, op, line).w;
      };
      auto append_line = [&](const std::string& line)
      {
        const int width = measure_width(line);
        layout.lines.push_back({line, width});
        layout.size.w = std::max(layout.size.w, width);
      };

      for (const auto& paragraph : split_paragraphs(text))
      {
        if (!should_wrap)
        {
          append_line(paragraph);
          continue;
        }

        std::string current;
        std::string pending_whitespace;
        bool emitted_line = false;
        bool at_paragraph_start = true;

        for (size_t i = 0; i < paragraph.size();)
        {
          while (i < paragraph.size() && is_wrap_whitespace(paragraph[i]))
          {
            pending_whitespace.push_back(paragraph[i]);
            i++;
          }

          const size_t word_start = i;
          while (i < paragraph.size() && !is_wrap_whitespace(paragraph[i]))
          {
            i++;
          }

          const std::string word = paragraph.substr(word_start, i - word_start);
          if (word.empty())
          {
            continue;
          }

          if (current.empty())
          {
            current = at_paragraph_start ? pending_whitespace + word : word;
            pending_whitespace.clear();
            at_paragraph_start = false;
            continue;
          }

          const std::string candidate = current + pending_whitespace + word;
          if (measure_width(candidate) <= *max_width)
          {
            current = candidate;
            pending_whitespace.clear();
            continue;
          }

          append_line(current);
          emitted_line = true;
          current = word;
          pending_whitespace.clear();
          at_paragraph_start = false;
        }

        if (!current.empty() && !pending_whitespace.empty())
        {
          current += pending_whitespace;
        }

        if (!current.empty())
        {
          append_line(current);
        }
        else if (!emitted_line)
        {
          append_line(paragraph);
        }
      }

      if (layout.lines.empty())
      {
        append_line("");
      }

      layout.size.h = line_height * static_cast<int>(layout.lines.size());
      return layout;
    }

    void render_text_line(RenderContext& rc,
                          const TextRenderOp& op,
                          const std::string& text,
                          int x,
                          int y)
    {
      PIXILS_BENCHMARK_COUNT(text_render_lines);
      auto mutable_op = op;
      int cursor_x = x;
      auto segments = split_inline_segments(text, mutable_op);
      PIXILS_BENCHMARK_ADD(text_render_segments,
                           static_cast<std::int64_t>(segments.size()));

      for (const auto& segment : segments)
      {
        auto& tint_renderer = select_tint_renderer(mutable_op, segment.use_inline_style);
        auto& renderer = select_renderer(mutable_op, segment.use_inline_style);
        const SDL_Color& color = select_color(mutable_op, segment.use_inline_style);
        const auto& shadows = select_shadows(mutable_op, segment.use_inline_style);

        for (const auto& shadow : shadows)
        {
          SDL_Color shadow_color = shadow.color.to_SDL_Color();
          tint_renderer.set_alt_color(shadow_color);
          tint_renderer.render_text(rc,
                                    segment.text,
                                    cursor_x + shadow.offset.x,
                                    y + shadow.offset.y,
                                    shadow_color);
          render_underlines(rc,
                            mutable_op,
                            segment.use_inline_style,
                            cursor_x + shadow.offset.x,
                            y + shadow.offset.y,
                            tint_renderer.get_rendered_size(rc, segment.text).w,
                            shadow_color);
        }

        renderer.render_text(rc, segment.text, cursor_x, y, color);
        int width = renderer.get_rendered_size(rc, segment.text).w;
        render_underlines(rc,
                          mutable_op,
                          segment.use_inline_style,
                          cursor_x,
                          y,
                          width,
                          color);
        cursor_x += width;
      }
    }

    void render_text(RenderContext& rc,
                     const TextRenderOp& op,
                     const std::string& text,
                     int x,
                     int y)
    {
      PIXILS_BENCHMARK_COUNT(text_render_calls);
      PIXILS_BENCHMARK_TIME_BLOCK(text_render_time_ns);
      const int line_height = op_line_height(op);
      auto layout = layout_text(rc, op, text, WrapMode::NONE, std::nullopt);
      for (size_t i = 0; i < layout.lines.size(); i++)
      {
        render_text_line(rc,
                         op,
                         layout.lines[i].text,
                         x,
                         y + static_cast<int>(i) * line_height);
      }
    }

    /**
     * Shadow
     **/
    Shadow::Shadow(const Shadow& other)
      : offset(other.offset)
      , color(other.color)
    {
    }

    Shadow::Shadow(const Point& offset, const Color& color)
      : offset(offset)
      , color(color)
    {
    }

    bool Shadow::operator==(const Shadow& other) const
    {
      return this->offset == other.offset && this->color == other.color;
    }

    /**
     * Cursor
     */
    Cursor::Cursor(Renderer& renderer, const SDL_Color& color, const SDL_Color& alt_color)
      : Cursor(renderer, {0, 0}, color, alt_color, renderer.get_line_height())
    {
    }

    Cursor::Cursor(Renderer& renderer,
                   const Point& position,
                   const SDL_Color& color,
                   int line_height)
      : Cursor(renderer, position, color, color, line_height)
    {
    }

    Cursor::Cursor(Renderer& renderer, const SDL_Color& color, int line_height)
      : Cursor(renderer, Point{0, 0}, color, line_height)
    {
    }

    Cursor::Cursor(Renderer& renderer,
                   const SDL_Color& color,
                   const SDL_Color& alt_color,
                   int line_height)
      : Cursor(renderer, Point{0, 0}, color, alt_color, line_height)
    {
    }

    Cursor::Cursor(Renderer& renderer,
                   const Point& position,
                   const SDL_Color& color,
                   const SDL_Color& alt_color,
                   int line_height)
      : renderer(renderer)
      , position(position)
      , color(color)
      , alt_color(alt_color)
      , line_height(line_height)
      , line_start_x(position.x)
    {
    }

    void Cursor::set_line_height(int line_height)
    {
      this->line_height = line_height;
    }

    int Cursor::get_line_height() const
    {
      return line_height;
    }

    void Cursor::set_alignment(Alignment alignment)
    {
      this->alignment = alignment;
    }

    void Cursor::render_text(RenderContext& rc,
                             const std::string& text,
                             const SDL_Color& color)
    {
      PIXILS_BENCHMARK_COUNT(text_render_calls);
      PIXILS_BENCHMARK_TIME_BLOCK(text_render_time_ns);
      PIXILS_BENCHMARK_COUNT(text_render_lines);
      int align_mod = 0;
      switch (alignment)
      {
      case Alignment::CENTER:
        align_mod -= marker_aware_rendered_size(rc, renderer, text).w / 2;
        break;
      case Alignment::RIGHT:
        align_mod -= marker_aware_rendered_size(rc, renderer, text).w;
        break;
      default:
        break;
      }

      auto render_segments = [&](int x, int y, const SDL_Color& base_color, bool use_alt)
      {
        int cursor_x = x;
        auto segments = split_marker_segments(text);
        PIXILS_BENCHMARK_ADD(text_render_segments,
                             static_cast<std::int64_t>(segments.size()));
        for (const auto& segment : segments)
        {
          const SDL_Color& segment_color =
            use_alt && segment.use_inline_style ? alt_color : base_color;
          renderer.render_text(rc, segment.text, cursor_x, y, segment_color);
          cursor_x += renderer.get_rendered_size(rc, segment.text).w;
        }
      };

      SDL_Color shadow_color;
      for (auto& shadow : shadows)
      {
        shadow_color = shadow.color.to_SDL_Color();
        render_segments(align_mod + position.x + (shadow.offset.x),
                        position.y + shadow.offset.y,
                        shadow_color,
                        false);
      }
      render_segments(align_mod + position.x, position.y, color, true);
    }

    SDL_Rect Cursor::get_rendered_rect(RenderContext& rc, const std::string& text)
    {
      SDL_Rect rect = marker_aware_rendered_size(rc, renderer, text);

      rect.x = position.x;
      rect.y = position.y;

      switch (alignment)
      {
      case Alignment::CENTER:
        rect.x -= rect.w / 2;
        break;
      case Alignment::RIGHT:
        rect.x -= rect.w - 1;
        break;
      default:
        break;
      }

      return rect;
    }

    void Cursor::add_shadow(const Shadow& shadow)
    {
      this->shadows.push_back(shadow);
    }

    void Cursor::print(RenderContext& rc, const std::string& text)
    {
      this->print(rc, text, this->color);
    }

    void Cursor::print(RenderContext& rc, const std::string& text, const SDL_Color& color)
    {
      render_text(rc, text, color);
      position.x += marker_aware_rendered_size(rc, renderer, text).w;
    }

    void Cursor::print(RenderContext& rc,
                       const std::string& text,
                       const SDL_Color& color,
                       const SDL_Color& background_color)
    {
      SDL_Rect bg_rect = get_rendered_rect(rc, text);
      bg_rect.x -= 1;
      bg_rect.y -= 1;
      bg_rect.w += 1;
      bg_rect.h += 1;

      if (bg_rect.x < 0)
      {
        position.x -= bg_rect.x;
        bg_rect.x -= bg_rect.x;
      }

      SDL_SetRenderDrawColor(rc.renderer,
                             background_color.r,
                             background_color.g,
                             background_color.b,
                             background_color.a);
      PIXILS_BENCHMARK_COUNT(render_fill_rect_calls);
      SDL_RenderFillRect(rc.renderer, &bg_rect);

      print(rc, text, color);
    }

    void Cursor::println(RenderContext& rc, const std::string& text)
    {
      this->println(rc, text, this->color);
    }

    void Cursor::println(RenderContext& rc, const std::string& text, const SDL_Color& color)
    {
      render_text(rc, text, color);
      position.y += line_height * renderer.get_scale_y();
      ;
      position.x = line_start_x;
    }

    void Cursor::set_color(const SDL_Color& color)
    {
      this->color = color;
    }

    void Cursor::move_to(int x, int y)
    {
      this->move_to({x, y});
    }

    void Cursor::move_to(const Point& coordinate)
    {
      this->position = coordinate;
      this->line_start_x = coordinate.x;
    }

    Alignment Cursor::get_alignment() const
    {
      return alignment;
    }

    const Point& Cursor::get_position() const
    {
      return this->position;
    }

    Renderer& Cursor::get_renderer()
    {
      return renderer;
    }

  } // namespace Text
} // namespace Pixils
