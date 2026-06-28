
#include <pixils/font_registry.h>

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <algorithm>
#include <cstdlib>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace Pixils
{
  namespace
  {
    struct RasterizedTtfFont
    {
      SDL_Texture* texture = nullptr;
      Text::FontMap map{std::map<char32_t, SDL_Rect>{}};
      Text::FontDefinition definition;
      int line_height = 0;
    };

    void put_pixel(SDL_Surface* surface, int x, int y, Uint8 alpha)
    {
      if (!surface || !surface->pixels || !surface->format) return;
      if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) return;

      auto* pixels = static_cast<Uint32*>(surface->pixels);
      const int stride = surface->pitch / static_cast<int>(sizeof(Uint32));
      pixels[(y * stride) + x] =
        SDL_MapRGBA(surface->format, 0xff, 0xff, 0xff, alpha);
    }

    RasterizedTtfFont rasterize_ttf_face(SDL_Renderer* renderer,
                                         FT_Face face,
                                         int size,
                                         Text::FontDefinition definition,
                                         int line_height,
                                         bool infer_baseline)
    {
      RasterizedTtfFont out;
      const int pixel_size = std::max(1, size);
      FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixel_size));

      const int ascender = static_cast<int>(face->size->metrics.ascender >> 6);
      const int descender = static_cast<int>((-face->size->metrics.descender) >> 6);
      const int metric_height = static_cast<int>(face->size->metrics.height >> 6);
      const int baseline = infer_baseline ? std::max(0, ascender)
                                          : definition.baseline;
      const int effective_line_height =
        std::max({line_height, metric_height, ascender + descender, pixel_size});

      if (infer_baseline) definition.baseline = baseline;
      out.definition = definition;
      out.line_height = effective_line_height;

      struct Glyph
      {
        char32_t ch;
        int width;
        int height;
        int bearing_left;
        int bearing_top;
        int advance;
        std::vector<unsigned char> buffer;
      };

      std::vector<Glyph> glyphs;
      glyphs.reserve(95);

      int atlas_width = 0;
      for (char32_t ch = 32; ch <= 126; ch++)
      {
        if (FT_Load_Char(face, ch, FT_LOAD_RENDER) != 0) continue;

        FT_GlyphSlot slot = face->glyph;
        Glyph glyph;
        glyph.ch = ch;
        glyph.width = static_cast<int>(slot->bitmap.width);
        glyph.height = static_cast<int>(slot->bitmap.rows);
        glyph.bearing_left = slot->bitmap_left;
        glyph.bearing_top = slot->bitmap_top;
        glyph.advance = std::max(1, static_cast<int>(slot->advance.x >> 6));
        const int pitch = std::abs(slot->bitmap.pitch);
        glyph.buffer.resize(static_cast<size_t>(glyph.width * glyph.height), 0);
        for (int y = 0; y < glyph.height; y++)
        {
          for (int x = 0; x < glyph.width; x++)
          {
            glyph.buffer[static_cast<size_t>((y * glyph.width) + x)] =
              slot->bitmap.buffer[static_cast<size_t>((y * pitch) + x)];
          }
        }

        const int left_pad = std::max(0, -glyph.bearing_left);
        const int cell_width =
          std::max(1,
                   std::max(glyph.advance + left_pad,
                            left_pad + glyph.bearing_left + glyph.width));
        atlas_width += cell_width;
        glyphs.push_back(std::move(glyph));
      }

      SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0,
                                                            std::max(1, atlas_width),
                                                            std::max(1, effective_line_height),
                                                            32,
                                                            SDL_PIXELFORMAT_RGBA8888);
      if (!surface)
      {
        return out;
      }

      if (surface->pixels && surface->format)
      {
        auto* pixels = static_cast<Uint32*>(surface->pixels);
        const int stride = surface->pitch / static_cast<int>(sizeof(Uint32));
        const Uint32 transparent = SDL_MapRGBA(surface->format, 0, 0, 0, 0);
        for (int y = 0; y < surface->h; y++)
        {
          for (int x = 0; x < surface->w; x++)
          {
            pixels[(y * stride) + x] = transparent;
          }
        }
      }

      int cursor_x = 0;
      std::map<char32_t, SDL_Rect> glyph_map;
      for (const auto& glyph : glyphs)
      {
        const int left_pad = std::max(0, -glyph.bearing_left);
        const int cell_width =
          std::max(1,
                   std::max(glyph.advance + left_pad,
                            left_pad + glyph.bearing_left + glyph.width));
        const int draw_x = cursor_x + left_pad + glyph.bearing_left;
        const int draw_y = baseline - glyph.bearing_top;

        for (int y = 0; y < glyph.height; y++)
        {
          for (int x = 0; x < glyph.width; x++)
          {
            put_pixel(surface,
                      draw_x + x,
                      draw_y + y,
                      glyph.buffer[static_cast<size_t>((y * glyph.width) + x)]);
          }
        }

        glyph_map.emplace(glyph.ch,
                          SDL_Rect{cursor_x, 0, cell_width, effective_line_height});
        cursor_x += cell_width;
      }

      try
      {
        out.texture = SDL_CreateTextureFromSurface(renderer, surface);
      }
      catch (...)
      {
        out.texture = SDL_CreateTexture(renderer,
                                        SDL_PIXELFORMAT_RGBA8888,
                                        SDL_TEXTUREACCESS_STATIC,
                                        surface->w,
                                        surface->h);
      }
      if (out.texture) SDL_SetTextureBlendMode(out.texture, SDL_BLENDMODE_BLEND);
      SDL_FreeSurface(surface);

      out.map = Text::FontMap(glyph_map);
      return out;
    }

    RasterizedTtfFont rasterize_ttf_font(SDL_Renderer* renderer,
                                         const std::string& file_name,
                                         int size,
                                         Text::FontDefinition definition,
                                         int line_height,
                                         bool infer_baseline)
    {
      FT_Library library = nullptr;
      if (FT_Init_FreeType(&library) != 0) return {};

      FT_Face face = nullptr;
      if (FT_New_Face(library, file_name.c_str(), 0, &face) != 0)
      {
        FT_Done_FreeType(library);
        return {};
      }

      RasterizedTtfFont out =
        rasterize_ttf_face(renderer,
                           face,
                           size,
                           std::move(definition),
                           line_height,
                           infer_baseline);
      FT_Done_Face(face);
      FT_Done_FreeType(library);
      return out;
    }

    RasterizedTtfFont rasterize_ttf_font_data(SDL_Renderer* renderer,
                                              const unsigned char* data,
                                              std::size_t data_size,
                                              int size,
                                              Text::FontDefinition definition,
                                              int line_height,
                                              bool infer_baseline)
    {
      if (!data || data_size == 0) return {};

      FT_Library library = nullptr;
      if (FT_Init_FreeType(&library) != 0) return {};

      FT_Face face = nullptr;
      if (FT_New_Memory_Face(library,
                             reinterpret_cast<const FT_Byte*>(data),
                             static_cast<FT_Long>(data_size),
                             0,
                             &face) != 0)
      {
        FT_Done_FreeType(library);
        return {};
      }

      RasterizedTtfFont out =
        rasterize_ttf_face(renderer,
                           face,
                           size,
                           std::move(definition),
                           line_height,
                           infer_baseline);
      FT_Done_Face(face);
      FT_Done_FreeType(library);
      return out;
    }
  } // namespace

  BitmapFont::BitmapFont(SDL_Texture* texture,
                         SDL_Texture* tint_texture,
                         Text::FontMap map,
                         Text::FontDefinition definition,
                         int spacing,
                         int line_height,
                         SDL_Texture* owned_texture)
    : font_map(std::move(map))
    , definition(std::move(definition))
    , renderer(texture, font_map, spacing, 1, line_height)
    , tint_renderer(tint_texture ? tint_texture : texture, font_map, spacing, 1, line_height)
    , owned_texture(owned_texture)
  {
  }

  BitmapFont::~BitmapFont()
  {
    if (owned_texture) SDL_DestroyTexture(owned_texture);
  }

  void FontRegistry::register_font(const std::string& key,
                                   SDL_Texture* texture,
                                   SDL_Texture* tint_texture,
                                   Text::FontMap map,
                                   Text::FontDefinition definition,
                                   int spacing,
                                   int line_height)
  {
    fonts[key] = std::make_unique<BitmapFont>(texture,
                                              tint_texture,
                                              std::move(map),
                                              std::move(definition),
                                              spacing,
                                              line_height);
    generation_++;
  }

  bool FontRegistry::register_ttf_font(const std::string& key,
                                       SDL_Renderer* renderer,
                                       const std::string& file_name,
                                       int size,
                                       Text::FontDefinition definition,
                                       int spacing,
                                       int line_height,
                                       bool infer_baseline)
  {
    RasterizedTtfFont font = rasterize_ttf_font(renderer,
                                                file_name,
                                                size,
                                                std::move(definition),
                                                line_height,
                                                infer_baseline);
    if (!font.texture) return false;

    SDL_Texture* texture = font.texture;
    fonts[key] = std::make_unique<BitmapFont>(texture,
                                              texture,
                                              std::move(font.map),
                                              std::move(font.definition),
                                              spacing,
                                              font.line_height,
                                              texture);
    generation_++;
    return true;
  }

  bool FontRegistry::register_ttf_font_data(const std::string& key,
                                            SDL_Renderer* renderer,
                                            const unsigned char* data,
                                            std::size_t size,
                                            int pixel_size,
                                            Text::FontDefinition definition,
                                            int spacing,
                                            int line_height,
                                            bool infer_baseline)
  {
    RasterizedTtfFont font = rasterize_ttf_font_data(renderer,
                                                     data,
                                                     size,
                                                     pixel_size,
                                                     std::move(definition),
                                                     line_height,
                                                     infer_baseline);
    if (!font.texture) return false;

    SDL_Texture* texture = font.texture;
    fonts[key] = std::make_unique<BitmapFont>(texture,
                                              texture,
                                              std::move(font.map),
                                              std::move(font.definition),
                                              spacing,
                                              font.line_height,
                                              texture);
    generation_++;
    return true;
  }

  BitmapFont* FontRegistry::get_font(const std::string& key)
  {
    auto it = fonts.find(key);
    return it != fonts.end() ? it->second.get() : nullptr;
  }

  bool FontRegistry::has_font(const std::string& key) const
  {
    return fonts.count(key) > 0;
  }

  std::uint64_t FontRegistry::generation() const
  {
    return generation_;
  }

} // namespace Pixils
