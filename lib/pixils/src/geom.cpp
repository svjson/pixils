
#include <pixils/geom.h>

#include <SDL2/SDL_pixels.h>
#include <algorithm>
#include <bits/std_abs.h>
#include <cmath>
#include <stdlib.h>

namespace Pixils
{
  const Point POINT__ZERO_ZERO(0, 0);

  /* Point */
  Point::Point(int x, int y)
    : x(x)
    , y(y)
  {
  }

  Point::Point(float x, float y)
    : x(x)
    , y(y)
  {
  }

  int Point::distance_to(const Point& other) const
  {
    return std::max(std::abs(x - other.x), std::abs(y - other.y));
  }

  float Point::euclidean_distance_to(const Point& other) const
  {
    return std::sqrt((other.x - this->x) * (other.x - this->x) +
                     (other.y - this->y) * (other.y - this->y));
  }

  Point Point::plus(float x, float y) const
  {
    return Point{this->x + x, this->y + y};
  }

  Point Point::operator+(const Point& other) const
  {
    return Point{this->x + other.x, this->y + other.y};
  }

  Point Point::operator+(float term) const
  {
    return Point{this->x + term, this->y + term};
  }

  Point Point::operator-(const Point& other) const
  {
    return Point{this->x - other.x, this->y - other.y};
  }

  Point Point::operator*(float multiplier) const
  {
    return Point(this->x * multiplier, this->y * multiplier);
  }

  bool Point::operator<(const Point& other) const
  {
    return y < other.y || (y == other.y && x < other.x);
  }

  bool Point::operator>(const Point& other) const
  {
    return y > other.y || (y == other.y && x > other.x);
  }

  Point Point::minus(float x, float y) const
  {
    return Point{this->x - x, this->y - y};
  }

  Point Point::rotate(const Point& origin, float amount) const
  {
    if (amount == 0.0) return *this;

    float s = std::sin(amount);
    float c = std::cos(amount);

    // translate point to origin
    float x = this->x - origin.x;
    float y = this->y - origin.y;

    // rotate
    float x_new = x * c - y * s;
    float y_new = x * s + y * c;

    // translate back
    x_new += origin.x;
    y_new += origin.y;

    return {x_new, y_new};
  }

  Point Point::floor() const
  {
    return Point(static_cast<int>(this->x), static_cast<int>(this->y));
  }

  Point Point::round() const
  {
    return Point(static_cast<int>(std::lround(this->x)),
                 static_cast<int>(std::lround(this->y)));
  }

  int Point::round_x() const
  {
    return static_cast<int>(std::lround(this->x));
  }

  int Point::round_y() const
  {
    return static_cast<int>(std::lround(this->y));
  }

  bool Point::operator==(const Point& other) const
  {
    return this->x == other.x && this->y == other.y;
  }

  std::ostream& operator<<(std::ostream& os, const Point& coordinate)
  {
    os << "{" << coordinate.x << ", " << coordinate.y << "}";
    return os;
  }

  /* Color */
  bool Color::operator==(const Color& other) const
  {
    return this->r == other.r && this->g == other.g && this->b == other.b &&
           this->a == other.a;
  }

  SDL_Color Color::to_SDL_Color() const
  {
    return SDL_Color{r, g, b, a};
  }

  /* Rect */
  bool Rect::operator==(const Rect& other) const
  {
    return x == other.x && y == other.y && w == other.w && h == other.h;
  }

  bool Rect::operator!=(const Rect& other) const
  {
    return !(*this == other);
  }

  Rect Rect::merge(const Rect& other) const
  {
    Rect result = *this;
    result.x = std::min(this->x, other.x);
    result.w = std::max(this->x + this->w, other.x + other.w) - result.x;

    result.y = std::min(this->y, other.y);
    result.h = std::max(this->y + this->h, other.y + other.h) - result.y;

    return result;
  }

  Point Rect::top_left() const
  {
    return Point(x, y);
  }

  Point Rect::bottom_right() const
  {
    return Point(x + w, y + h);
  }

  bool Rect::contains(const Point& coord) const
  {
    return contains(coord.x, coord.y);
  }

  bool Rect::contains(int x, int y) const
  {
    return x >= this->x && x < this->x + this->w && y >= this->y && y < this->y + this->h;
  }

  Rect expand_rect(const Rect* rect, float amount)
  {
    return expand_rect(rect, amount, amount);
  }

  Rect expand_rect(const Rect* rect, int x_amount, int y_amount)
  {
    return {rect->x - x_amount,
            rect->y - y_amount,
            rect->w + 2 * x_amount,
            rect->h + 2 * y_amount};
  }

  Rect shrink_rect(const Rect& rect, int amount)
  {
    return shrink_rect(rect, amount, amount);
  }

  Rect shrink_rect(const Rect& rect, int x_amount, int y_amount)
  {
    return {rect.x + x_amount,
            rect.y + y_amount,
            rect.w - 2 * x_amount,
            rect.h - 2 * y_amount};
  }

  SDL_Rect Rect::to_SDL_rect() const
  {
    return {this->x, this->y, this->w, this->h};
  }

  /* Dimension */
  Dimension::Dimension(int w, int h)
    : w(w)
    , h(h)
  {
  }

  bool Dimension::operator==(const Dimension& other) const
  {
    return w == other.w && h == other.h;
  }

  bool Dimension::operator!=(const Dimension& other) const
  {
    return !(*this == other);
  }

  Rect trunc_rect(const Rect& rect, const Rect& limit)
  {
    Rect result{std::max(limit.x, std::min(rect.x, limit.x + limit.w - 1)),
                std::max(limit.y, std::min(rect.y, limit.y + limit.h - 1)),
                0,
                0};

    // Calculate the maximum possible width within the limit
    int maxWidth = std::min(limit.x + limit.w, rect.x + rect.w) - result.x;
    result.w = std::max(0, maxWidth); // Ensure non-negative width

    // Calculate the maximum possible height within the limit
    int maxHeight = std::min(limit.y + limit.h, rect.y + rect.h) - result.y;
    result.h = std::max(0, maxHeight); // Ensure non-negative height

    return result;
  }

} // namespace Pixils
