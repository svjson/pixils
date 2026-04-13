#ifndef PIXILS__ASSET__EMBEDDED_ASSETS_H
#define PIXILS__ASSET__EMBEDDED_ASSETS_H

#include <cstddef>

namespace Pixils::Assets
{
  struct EmbeddedAsset
  {
    const unsigned char* data;
    std::size_t size;
  };

  extern const EmbeddedAsset consolefont_png;
  extern const EmbeddedAsset pixils_logo_png;

} // namespace Pixils::Assets

#endif /* PIXILS__ASSET__EMBEDDED_ASSETS_H */
