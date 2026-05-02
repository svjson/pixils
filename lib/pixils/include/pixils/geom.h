
#ifndef __PIXILS__GEOM_H_
#define __PIXILS__GEOM_H_

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <ostream>

namespace Pixils
{
  /*!
   * @brief Represents an integer Point in any 2D plane.
   *
   * Mostly used to track and refer to tiles on the game map,
   * but can also be used for screen points.
   */
  struct Point
  {
    /*! @brief The horizontal point */
    float x = 0;
    /*! @brief The vertical point */
    float y = 0;

    /*!
     * @brief Creates a point with the default value of 0, 0
     */
    Point() = default;
    /*!
     * @brief Creates a point with the value of x and y
     */
    Point(int x, int y);
    /*!
     * @brief Creates a point with the value of x and y
     */
    Point(float x, float y);

    /*!
     * @brief Calculates the distance between this point
     * and other.
     */
    int distance_to(const Point& other) const;

    /*!
     * @brief Calculates the Euclidean distance between this point and
     * other.
     */
    float euclidean_distance_to(const Point& other) const;

    Point floor() const;
    Point round() const;

    int round_x() const;
    int round_y() const;

    Point operator+(const Point& other) const;
    Point operator+(float term) const;
    Point plus(float x, float y) const;

    Point operator-(const Point& other) const;
    Point minus(float x, float y) const;

    Point operator*(float multiplier) const;

    Point rotate(const Point& origin, float amount) const;

    bool operator<(const Point& other) const;
    bool operator>(const Point& other) const;

    bool operator==(const Point& other) const;
  };

  std::ostream& operator<<(std::ostream&, const Point& point);

  extern const Point POINT__ZERO_ZERO;

  /*!
   * @brief Describes the size of a 2-dimensional plane.
   */
  struct Dimension
  {
    /*! @brief Width, the horizontal size */
    int w = 0;
    /*! @brief Height, the vertical size */
    int h = 0;

    /*! @brief Constructs a Dimension with the size of 0, 0 */
    Dimension() = default;
    /*! @brief Constructs a Dimension with the size of w, h */
    Dimension(int w, int h);

    bool operator==(const Dimension& other) const;
    bool operator!=(const Dimension& other) const;
  };

  /*!
   * @brief Describes the size and location of a 2-dimensional
   * rectangle.
   */
  struct Rect
  {
    /*! @brief The horizontal point of the top-left corner of the rect */
    int x;
    /*! @brief The vertical point of the top-left corner of the rect */
    int y;
    /*! @brief The width of the rect */
    int w;
    /*! @brief The height of the rect */
    int h;

    /*!
     * @brief Queries if another Rect intersects this Rect.
     */
    bool intersects(const Rect& other) const;

    Rect merge(const Rect& other) const;
    /*!
     * @brief Queries if a point is contained
     * within the bounds of this Rect.
     */
    bool contains(const Point& coord) const;
    /*!
     * @brief Queries if a point is contained
     * within the bounds of this Rect.
     */
    bool contains(int x, int y) const;
    /*!
     * @brief Get the top left corner, the x and y values
     * as Point.
     */
    Point top_left() const;
    Point top_right() const;
    Point bottom_right() const;
    Point bottom_left() const;

    SDL_Rect to_SDL_rect() const;

    bool operator==(const Rect& other) const;
    bool operator!=(const Rect& other) const;
  };

  Rect expand_rect(const Rect* rect, int amount);
  Rect expand_rect(const Rect* rect, int x_amount, int y_amount);
  Rect shrink_rect(const Rect& rect, int amount);
  Rect shrink_rect(const Rect& rect, int x_amount, int y_amount);

  Rect trunc_rect(const Rect& rect, const Rect& limit);

  /*!
   * @brief Describes the red, green and blue balance of a
   * color, as well as its opacity.
   *
   * All values range between 0-255.
   * a=0 is complete transparency.
   * a=255 is complete opaqueness.
   */
  struct Color
  {
    /*! @brief The amount of red, 0-255 */
    uint8_t r = 0x00;
    /*! @brief The amount of green, 0-255 */
    uint8_t g = 0x00;
    /*! @brief The amount of blue, 0-255 */
    uint8_t b = 0x00;
    /*! @brief The alpha channel value, 0-255 */
    uint8_t a = 0xff;

    bool operator==(const Color& other) const;

    /*!
     * @brief Creates the SDL equivalent.
     */
    SDL_Color to_SDL_Color() const;
  };

} // namespace Pixils

#endif
