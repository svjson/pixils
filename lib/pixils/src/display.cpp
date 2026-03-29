
#include <pixils/display.h>

namespace Pixils
{
  Resolution::Resolution(const Resolution& other)
    : mode(other.mode)
    , dimension(other.dimension)
  {
  }
  Resolution::Resolution(Mode mode)
    : mode(mode)
    , dimension({0, 0})
  {
  }

  Resolution::Resolution(Mode mode, const Dimension& dimension)
    : mode(mode)
    , dimension(dimension)
  {
  }

  void Resolution::operator=(const Resolution& other)
  {
    this->mode = other.mode;
    this->dimension = other.dimension;
  }

  Display::Display(const Resolution& resolution,
                   Alignment align,
                   Scaling scaling,
                   const Color& background)
    : resolution(resolution)
    , align(align)
    , scaling(scaling)
    , background(background)
  {
  }

  Display::Display(const Display& other)
    : resolution(other.resolution)
    , align(other.align)
    , scaling(other.scaling)
    , background(other.background)
  {
  }

  void Display::operator=(const Display& other)
  {
    this->resolution = other.resolution;
    this->align = other.align;
    this->scaling = other.scaling;
    this->background = other.background;
  }

} // namespace Pixils
