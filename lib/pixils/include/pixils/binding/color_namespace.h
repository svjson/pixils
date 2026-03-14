
#ifndef __PIXILS__COLOR_NAMESPACE_H_
#define __PIXILS__COLOR_NAMESPACE_H_

#include <pixils/geom.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>
#include <string>

namespace Pixils
{
  namespace Script
  {
    /*!
     * @brief Constant for "pixils.color"
     */
    inline const std::string NS__PIXILS__COLOR = "pixils.color";

    /*!
     * @brief MakeColor function name
     */
    inline const std::string FN__MAKE_COLOR = "make-color";
    inline const std::string FN__PIXILS__COLOR__MAKE_COLOR = "pixils.color/" + FN__MAKE_COLOR;

    /*!
     * @brief WithAlpha function name
     */
    inline const std::string FN__WITH_ALPHA = "with-alpha";

    namespace MapKey
    {
      DECL_SHKEY(A);
      DECL_SHKEY(B);
      DECL_SHKEY(G);
      DECL_SHKEY(R);
    } // namespace MapKey

    namespace HostType
    {
      HOST_TYPE(COLOR, "Color", FN__PIXILS__COLOR__MAKE_COLOR)
    }

    /*! @brief ColorAdapter - A Lisple HostObject Adapter for Color */
    HOST_ADAPTER(ColorAdapter, Color, (r, g, b, a), (r, g, b, a));

    namespace Function
    {
      /*!
       * @brief Lisple Function that constructs a new instance of Color/ColorAdapter
       */
      FUNC_DECL(MakeColor, make_color);
      /*!
       * @brief Constructs a new color with new alpha channel value
       */
      FUNC_DECL(WithAlpha, with_alpha);
    } // namespace Function

    class ColorNamespace : public Lisple::Namespace
    {
    public:
      ColorNamespace();
    };
  } // namespace Script
} // namespace Pixils

#endif /* __PIXILS__COLOR_NAMESPACE_H_ */
