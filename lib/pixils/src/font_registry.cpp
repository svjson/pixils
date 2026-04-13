
#include <pixils/font_registry.h>

namespace Pixils
{
  BitmapFont::BitmapFont(SDL_Texture* texture, Text::FontMap map, int spacing)
    : font_map(std::move(map))
    , renderer(texture, font_map, spacing, 1)
  {
  }

  void FontRegistry::register_font(const std::string& key,
                                   SDL_Texture* texture,
                                   Text::FontMap map,
                                   int spacing)
  {
    fonts.emplace(key, std::make_unique<BitmapFont>(texture, std::move(map), spacing));
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
