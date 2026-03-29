
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

    Resolution(const Resolution& other);
    Resolution(Mode mode);
    Resolution(Mode mode, const Dimension& dimension);

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
