#ifndef PIXILS__ASSET__BUNDLE_H
#define PIXILS__ASSET__BUNDLE_H

#include <stdint.h>
#include <string>
#include <unordered_map>

struct SDL_Texture;

namespace Pixils::Asset
{
  struct Bundle
  {
    std::unordered_map<std::string, SDL_Texture*> images;
  };
} // namespace Pixils::Asset

#endif /* PIXILS__ASSET__BUNDLE_H */
