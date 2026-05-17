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
  extern const EmbeddedAsset autoega_8x14_ttf;
  extern const EmbeddedAsset pixils_logo_png;
  extern const EmbeddedAsset win311_checkmark_png;
  extern const EmbeddedAsset win311_control_button_png;
  extern const EmbeddedAsset win311_minimize_button_png;
  extern const EmbeddedAsset win311systemfont_png;
  extern const EmbeddedAsset win95_minimize_button_png;
  extern const EmbeddedAsset win95systemfont_png;

} // namespace Pixils::Assets

#endif /* PIXILS__ASSET__EMBEDDED_ASSETS_H */
