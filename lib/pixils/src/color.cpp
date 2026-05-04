#include <pixils/color.h>
#include <pixils/num.h>

#include <stdexcept>

namespace Pixils
{
  Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : r(r)
    , g(g)
    , b(b)
    , a(a)
  {
  }

  bool Color::operator==(const Color& other) const
  {
    return this->r == other.r && this->g == other.g && this->b == other.b &&
           this->a == other.a;
  }

  SDL_Color Color::to_SDL_Color() const
  {
    return SDL_Color{r, g, b, a};
  }

  Color Color::from_hex_string(std::string_view spec)
  {
    if (spec.empty() || spec[0] != '#')
    {
      throw std::runtime_error("Color string must start with '#' and use CSS hex notation");
    }

    const std::string_view hex = spec.substr(1);
    switch (hex.size())
    {
    case 3:
      return Color{Num::expand_hex_nibble(hex[0]),
                   Num::expand_hex_nibble(hex[1]),
                   Num::expand_hex_nibble(hex[2]),
                   0xFF};
    case 4:
      return Color{Num::expand_hex_nibble(hex[0]),
                   Num::expand_hex_nibble(hex[1]),
                   Num::expand_hex_nibble(hex[2]),
                   Num::expand_hex_nibble(hex[3])};
    case 6:
      return Color{Num::parse_hex_byte(hex[0], hex[1]),
                   Num::parse_hex_byte(hex[2], hex[3]),
                   Num::parse_hex_byte(hex[4], hex[5]),
                   0xFF};
    case 8:
      return Color{Num::parse_hex_byte(hex[0], hex[1]),
                   Num::parse_hex_byte(hex[2], hex[3]),
                   Num::parse_hex_byte(hex[4], hex[5]),
                   Num::parse_hex_byte(hex[6], hex[7])};
    default:
      throw std::runtime_error("Color string must use #RGB, #RGBA, #RRGGBB or #RRGGBBAA");
    }
  }
} // namespace Pixils
