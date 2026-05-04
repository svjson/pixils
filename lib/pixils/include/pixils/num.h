#ifndef PIXILS__NUM_H
#define PIXILS__NUM_H

#include <cstdint>

namespace Pixils::Num
{
  int parse_hex_digit(char ch);
  uint8_t parse_hex_byte(char high, char low);
  uint8_t expand_hex_nibble(char ch);
} // namespace Pixils::Num

#endif /* PIXILS__NUM_H */
