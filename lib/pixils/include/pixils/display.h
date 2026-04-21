
#ifndef PIXILS__DISPLAY_H
#define PIXILS__DISPLAY_H

#include "geom.h"

#include <cstdint>

namespace Pixils
{
  class Resolution
  {
   public:
    enum class Mode : uint8_t
    {
      FIXED,
      AUTO
    };

    Mode mode;
    Dimension dimension;
    /** Pixel scale for AUTO mode: buffer = window / pixel_scale. 1 = 1:1. */
    int pixel_scale = 1;

    Resolution(const Resolution& other);
    Resolution(Mode mode);
    Resolution(Mode mode, const Dimension& dimension);
    Resolution(Mode mode, int pixel_scale);

    void operator=(const Resolution& other);
  };

  class Display
  {
   public:
    enum class Alignment : uint8_t
    {
      NONE,
      CENTER
    };

    enum class Scaling : uint8_t
    {
      NONE,
      FIT,
      STRETCH
    };

    Resolution resolution;
    Alignment align;
    Scaling scaling;
    Color background;

    Display(const Resolution& resolution,
            Alignment align,
            Scaling scaling,
            const Color& background);
    Display(const Display& other);

    void operator=(const Display& other);
  };

} // namespace Pixils

#endif /* PIXILS__DISPLAY_H */
