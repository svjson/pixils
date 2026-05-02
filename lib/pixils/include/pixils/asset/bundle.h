#ifndef PIXILS__ASSET__BUNDLE_H
#define PIXILS__ASSET__BUNDLE_H

#include <stdint.h>
#include <string>
#include <unordered_map>

struct SDL_Texture;
struct Mix_Chunk;

namespace Pixils::Asset
{
  struct Bundle
  {
    std::unordered_map<std::string, SDL_Texture*> images;
    std::unordered_map<std::string, Mix_Chunk*> sounds;
  };
} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__BUNDLE_H */
