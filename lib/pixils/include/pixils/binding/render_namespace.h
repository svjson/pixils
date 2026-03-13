
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
    inline const std::string FN__USE_COLOR_BANG = "use-color!";

    namespace Function
    {
      /*! @brief draw-line! function */
      FUNC_DECL(DrawLineBang, draw_line);
      /*! @brief draw-polygon! function */
      FUNC_DECL(DrawPolygonBang, draw_polygon, draw_polygon_with_opts);
      /*! @brief use-color! function */
      FUNC_DECL(UseColorBang, use_color);
    } // namespace Function

    class RenderNamespace : public Lisple::Namespace
    {
    public:
      RenderNamespace();
    };
  } // namespace Script
} // namespace Pixils

#endif /* __PIXILS__RENDER_NAMESPACE_H_ */
