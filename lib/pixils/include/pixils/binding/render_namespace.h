
#ifndef PIXILS__RENDER_NAMESPACE_H
#define PIXILS__RENDER_NAMESPACE_H

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__RENDER = "pixils.render";

  inline constexpr std::string_view FN__DRAW_IMAGE_BANG = "image!";
  inline constexpr std::string_view FN__DRAW_LINE_BANG = "line!";
  inline constexpr std::string_view FN__DRAW_POLYGON_BANG = "polygon!";
  inline constexpr std::string_view FN__DRAW_RECT_BANG = "rect!";
  inline constexpr std::string_view FN__USE_COLOR_BANG = "use-color!";

  namespace MapKey
  {
    DECL_SHKEY(CLOSE);
    DECL_SHKEY(OFFSET);
    DECL_SHKEY(ROTATION);
  } // namespace MapKey

  namespace Function
  {
    /*! @brief draw-image! function */
    FUNC(DrawImageBang, draw_img);
    /*! @brief draw-line! function */
    FUNC(DrawLineBang, draw_line);
    /*! @brief draw-polygon! function */
    FUNC(DrawPolygonBang, polygon, polygon_with_opts);
    /*! @brief draw-rect! function */
    FUNC(DrawRectBang, draw_rect_from_points);
    /*! @brief use-color! function */
    FUNC(UseColorBang, use_color, use_color_num);
  } // namespace Function

  class RenderNamespace : public Lisple::Namespace
  {
   public:
    RenderNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__RENDER_NAMESPACE_H */
