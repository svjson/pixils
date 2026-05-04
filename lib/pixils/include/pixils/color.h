#ifndef PIXILS__COLOR_H
#define PIXILS__COLOR_H

#include <SDL2/SDL_pixels.h>
#include <cstdint>
#include <string_view>

namespace Pixils
{
  struct Color
  {
    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff);

    uint8_t r = 0x00;
    uint8_t g = 0x00;
    uint8_t b = 0x00;
    uint8_t a = 0xff;

    bool operator==(const Color& other) const;
    SDL_Color to_SDL_Color() const;

    static Color from_hex_string(std::string_view spec);
  };
} // namespace Pixils

#endif /* PIXILS__COLOR_H */
