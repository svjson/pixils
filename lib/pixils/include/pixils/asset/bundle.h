#ifndef PIXILS__ASSET__BUNDLE_H
#define PIXILS__ASSET__BUNDLE_H

#include <stdint.h>
#include <string>
#include <unordered_map>

struct SDL_Texture;
struct SDL_Surface;
struct MIX_Audio;

namespace Pixils::Assets
{
  struct EmbeddedAsset;
}

namespace Pixils::Asset
{
  struct Bundle
  {
    std::unordered_map<std::string, SDL_Texture*> images;
    std::unordered_map<std::string, SDL_Surface*> image_sources;
    std::unordered_map<std::string, SDL_Texture*> tint_masks;
    std::unordered_map<std::string, MIX_Audio*> sounds;
    std::unordered_map<std::string, std::string> fonts;
    std::unordered_map<std::string, const Assets::EmbeddedAsset*> embedded_fonts;
  };
} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__BUNDLE_H */
