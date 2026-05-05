#ifndef PIXILS__UI__LINE_H
#define PIXILS__UI__LINE_H

#include <pixils/geom.h>
#include <pixils/ui/style.h>

#include <optional>

struct SDL_Renderer;

namespace Pixils::UI
{
  enum class Edge : uint8_t
  {
    TOP,
    RIGHT,
    BOTTOM,
    LEFT,
  };

  enum class Corner : uint8_t
  {
    TOP_LEFT,
    BOTTOM_RIGHT,
  };

  struct LineSpec
  {
    int thickness = 0;
    std::optional<Color> color = std::nullopt;
    Style::LineStyle style = Style::LineStyle::SOLID;
    int trim_start = 0;
    int trim_end = 0;
  };

  void render_edge(SDL_Renderer* renderer,
                   const Rect& bounds,
                   Edge edge,
                   const LineSpec& spec);

  void render_bevel_corner(SDL_Renderer* renderer,
                           const Rect& bounds,
                           Corner corner,
                           const LineSpec& horizontal,
                           const LineSpec& vertical);
} // namespace Pixils::UI

#endif /* PIXILS__UI__LINE_H */
