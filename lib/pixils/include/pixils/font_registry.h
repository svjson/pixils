#ifndef PIXILS__FONT_REGISTRY_H
#define PIXILS__FONT_REGISTRY_H

#include <pixils/text.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;

namespace Pixils
{
  /**
   * Owns a bitmap font: the glyph map and the renderer that uses it.
   *
   * Non-copyable and non-movable because Renderer holds a reference
   * to font_map; the two fields must stay co-located.
   */
  struct BitmapFont
  {
    Text::FontMap font_map;
    Text::FontDefinition definition;
    Text::Renderer renderer;
    Text::Renderer tint_renderer;
    SDL_Texture* owned_texture = nullptr;

    BitmapFont(SDL_Texture* texture,
               SDL_Texture* tint_texture,
               Text::FontMap map,
               Text::FontDefinition definition = {},
               int spacing = 1,
               int line_height = 0,
               SDL_Texture* owned_texture = nullptr);

    ~BitmapFont();

    BitmapFont(const BitmapFont&) = delete;
    BitmapFont& operator=(const BitmapFont&) = delete;
    BitmapFont(BitmapFont&&) = delete;
    BitmapFont& operator=(BitmapFont&&) = delete;
  };

  /**
   * Registry of named bitmap fonts. Fonts are keyed by their fully-qualified
   * Roo keyword name without the leading colon (e.g. "font/console").
   *
   * The built-in console font is registered under "font/console" at startup
   * and is used as the default when no font is specified in a render call.
   */
  class FontRegistry
  {
    std::unordered_map<std::string, std::unique_ptr<BitmapFont>> fonts;
    std::uint64_t generation_ = 1;

   public:
    void register_font(const std::string& key,
                       SDL_Texture* texture,
                       SDL_Texture* tint_texture,
                       Text::FontMap map,
                       Text::FontDefinition definition = {},
                       int spacing = 1,
                       int line_height = 0);

    bool register_ttf_font(const std::string& key,
                           SDL_Renderer* renderer,
                           const std::string& file_name,
                           int size,
                           Text::FontDefinition definition = {},
                           int spacing = 1,
                           int line_height = 0,
                           bool infer_baseline = true);

    bool register_ttf_font_data(const std::string& key,
                                SDL_Renderer* renderer,
                                const unsigned char* data,
                                std::size_t size,
                                int pixel_size,
                                Text::FontDefinition definition = {},
                                int spacing = 1,
                                int line_height = 0,
                                bool infer_baseline = true);

    BitmapFont* get_font(const std::string& key);
    bool has_font(const std::string& key) const;
    std::uint64_t generation() const;
  };

} // namespace Pixils

#endif /* PIXILS__FONT_REGISTRY_H */
