
#ifndef PIXILS__COLOR_NAMESPACE_H
#define PIXILS__COLOR_NAMESPACE_H

#include <pixils/geom.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>
#include <string>

namespace Pixils::Script
{

  /*!
   * @brief Constant for "pixils.color"
   */
  inline constexpr std::string_view NS__PIXILS__COLOR = "pixils.color";

  /*!
   * @brief MakeColor function name
   */
  inline constexpr std::string_view FN__MAKE_COLOR = "make-color";
  inline constexpr std::string_view FN__PIXILS__COLOR__MAKE_COLOR = "pixils.color/make-color";

  /*!
   * @brief WithAlpha function name
   */
  inline constexpr std::string_view FN__WITH_ALPHA = "with-alpha";

  namespace MapKey
  {
    DECL_SHKEY(A);
    DECL_SHKEY(B);
    DECL_SHKEY(G);
    DECL_SHKEY(R);
  } // namespace MapKey

  namespace HostType
  {
    HOST_TYPE(COLOR, "Color", std::string(FN__PIXILS__COLOR__MAKE_COLOR))
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
} // namespace Pixils::Script

#endif /* PIXILS__COLOR_NAMESPACE_H */
