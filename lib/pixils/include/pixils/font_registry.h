#ifndef PIXILS__FONT_REGISTRY_H
#define PIXILS__FONT_REGISTRY_H

#include <pixils/text.h>

#include <memory>
#include <string>
#include <unordered_map>

typedef struct SDL_Texture SDL_Texture;

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
    Text::Renderer renderer;

    BitmapFont(SDL_Texture* texture, Text::FontMap map, int spacing = 1);

    BitmapFont(const BitmapFont&) = delete;
    BitmapFont& operator=(const BitmapFont&) = delete;
    BitmapFont(BitmapFont&&) = delete;
    BitmapFont& operator=(BitmapFont&&) = delete;
  };

  /**
   * Registry of named bitmap fonts. Fonts are keyed by their fully-qualified
   * Lisple keyword name without the leading colon (e.g. "font/console").
   *
   * The built-in console font is registered under "font/console" at startup
   * and is used as the default when no font is specified in a render call.
   */
  class FontRegistry
  {
    std::unordered_map<std::string, std::unique_ptr<BitmapFont>> fonts;

   public:
    void register_font(const std::string& key,
                       SDL_Texture* texture,
                       Text::FontMap map,
                       int spacing = 1);

    BitmapFont* get_font(const std::string& key);
    bool has_font(const std::string& key) const;
  };

} // namespace Pixils

#endif /* PIXILS__FONT_REGISTRY_H */
