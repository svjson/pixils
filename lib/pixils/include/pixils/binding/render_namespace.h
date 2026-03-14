
#ifndef __PIXILS__RENDER_NAMESPACE_H_
#define __PIXILS__RENDER_NAMESPACE_H_

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils
{
  namespace Script
  {
    inline const std::string NS__PIXILS__RENDER = "pixils.render";

    inline const std::string FN__DRAW_LINE_BANG = "line!";
    inline const std::string FN__DRAW_POLYGON_BANG = "polygon!";
    inline const std::string FN__DRAW_RECT_BANG = "rect!";
    inline const std::string FN__USE_COLOR_BANG = "use-color!";

    namespace MapKey
    {
      DECL_SHKEY(CLOSE);
      DECL_SHKEY(OFFSET);
      DECL_SHKEY(ROTATION);
    } // namespace MapKey

    namespace Function
    {
      /*! @brief draw-line! function */
      FUNC_DECL(DrawLineBang, draw_line);
      /*! @brief draw-polygon! function */
      FUNC_DECL(DrawPolygonBang, draw_polygon, draw_polygon_with_opts);
      /*! @brief draw-rect! function */
      FUNC_DECL(DrawRectBang, draw_rect_from_points);
      /*! @brief use-color! function */
      FUNC_DECL(UseColorBang, use_color, use_color_num);
    } // namespace Function

    class RenderNamespace : public Lisple::Namespace
    {
    public:
      RenderNamespace();
    };
  } // namespace Script
} // namespace Pixils

#endif /* __PIXILS__RENDER_NAMESPACE_H_ */
