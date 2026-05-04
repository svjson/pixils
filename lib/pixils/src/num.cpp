#include <pixils/num.h>

#include <stdexcept>

namespace Pixils::Num
{
  int parse_hex_digit(char ch)
  {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    return -1;
  }

  uint8_t parse_hex_byte(char high, char low)
  {
    const int hi = parse_hex_digit(high);
    const int lo = parse_hex_digit(low);

    if (hi < 0 || lo < 0)
    {
      throw std::runtime_error("Invalid hex digit in color string");
    }

    return static_cast<uint8_t>((hi << 4) | lo);
  }

  uint8_t expand_hex_nibble(char ch)
  {
    const int value = parse_hex_digit(ch);
    if (value < 0)
    {
      throw std::runtime_error("Invalid hex digit in color string");
    }

    return static_cast<uint8_t>((value << 4) | value);
  }
} // namespace Pixils::Num
