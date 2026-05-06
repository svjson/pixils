
#include <pixils/font_registry.h>

namespace Pixils
{
  BitmapFont::BitmapFont(SDL_Texture* texture,
                         SDL_Texture* tint_texture,
                         Text::FontMap map,
                         int spacing,
                         int line_height)
    : font_map(std::move(map))
    , renderer(texture, font_map, spacing, 1, line_height)
    , tint_renderer(tint_texture ? tint_texture : texture, font_map, spacing, 1, line_height)
  {
  }

  void FontRegistry::register_font(const std::string& key,
                                   SDL_Texture* texture,
                                   SDL_Texture* tint_texture,
                                   Text::FontMap map,
                                   int spacing,
                                   int line_height)
  {
    fonts.emplace(key,
                  std::make_unique<BitmapFont>(texture,
                                               tint_texture,
                                               std::move(map),
                                               spacing,
                                               line_height));
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

} // namespace Pixils
