
#include <pixils/context.h>
#include <pixils/geom.h>
#include <pixils/text.h>

#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <stddef.h>

namespace Pixils
{
  namespace Text
  {
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
    Renderer::Renderer(SDL_Texture* font, FontMap& font_map, int spacing, int scale)
      : font(font)
      , font_map(font_map)
      , spacing(spacing)
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

      bool color_switch = false;

      SDL_SetTextureColorMod(font, color.r, color.g, color.b);
      SDL_SetTextureAlphaMod(font, color.a);

      for (size_t i = 0; i < text.size(); i++)
      {
        char c = text.at(i);
        if (c == '@')
        {
          color_switch = !color_switch;
          if (color_switch)
          {
            SDL_SetTextureColorMod(font, alt_color.r, alt_color.g, alt_color.b);
          }
          else
          {
            SDL_SetTextureColorMod(font, color.r, color.g, color.b);
          }
        }
        else
        {
          if (!font_map.has_char(c))
          {
            if (c == 10)
              c = ' ';
            else
              continue;
          }
          const SDL_Rect& char_rect = *font_map.get_char_rect(c);
          cursor.w = char_rect.w * scale;
          cursor.h = char_rect.h * scale;

          SDL_RenderCopy(rc.renderer, font, &char_rect, &cursor);
          cursor.x += cursor.w + (spacing * scale);
        }
      }
    }

    SDL_Rect Renderer::get_rendered_size(RenderContext&, const std::string& string) const
    {
      SDL_Rect rect{0, 0, 0, 0};

      for (size_t i = 0; i < string.size(); i++)
      {
        char c = string.at(i);
        if (c != '@' && !font_map.has_char(c)) c = ' ';

        if (c != '@')
        {
          const SDL_Rect& char_rect = *font_map.get_char_rect(c);
          rect.w += char_rect.w + spacing;
          rect.h = std::max(rect.h, char_rect.h);
        }
      }

      rect.w *= scale;
      rect.h *= scale;

      return rect;
    }

    void Renderer::set_scale(int scale)
    {
      this->scale = std::max(1, scale);
    }

    int Renderer::get_scale() const
    {
      return this->scale;
    }

    int Renderer::get_font_height() const
    {
      return font_map.get_char_rect(font_map.keys().front())->h;
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
      : Cursor(renderer, {0, 0}, color, alt_color, renderer.get_font_height())
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
      int align_mod = 0;
      switch (alignment)
      {
      case Alignment::CENTER:
        align_mod -= renderer.get_rendered_size(rc, text).w / 2;
        break;
      case Alignment::RIGHT:
        align_mod -= renderer.get_rendered_size(rc, text).w;
        break;
      default:
        break;
      }

      SDL_Color shadow_color;
      for (auto& shadow : shadows)
      {
        shadow_color = shadow.color.to_SDL_Color();
        renderer.set_alt_color(shadow_color);
        renderer.render_text(rc,
                             text,
                             align_mod + position.x + (shadow.offset.x),
                             position.y + shadow.offset.y,
                             shadow_color);
      }
      renderer.set_alt_color(alt_color);
      renderer.render_text(rc, text, align_mod + position.x, position.y, color);
    }

    SDL_Rect Cursor::get_rendered_rect(RenderContext& rc, const std::string& text)
    {
      SDL_Rect rect = renderer.get_rendered_size(rc, text);

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
      position.x += renderer.get_rendered_size(rc, text).w;
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
      position.y += line_height * renderer.get_scale();
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
