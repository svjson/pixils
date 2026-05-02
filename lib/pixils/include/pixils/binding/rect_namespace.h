
#ifndef PIXILS__RECT_NAMESPACE_H
#define PIXILS__RECT_NAMESPACE_H

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <string>

namespace Pixils
{
  struct Rect;
}

namespace Pixils::Script
{
  /*!
   * @brief Constant for "pixils.point" lisple namespace name
   */
  inline constexpr std::string_view NS__PIXILS__RECT = "pixils.rect";

  inline const std::string FN__PIXILS__MAKE_RECT = "pixils.rect/make-rect";

  namespace HostType
  {
    HOST_TYPE(RECT, "HRect", FN__PIXILS__MAKE_RECT);
  }

  namespace Function
  {
    /*! @brief Lisple make-function for Rect/RectAdapter */
    FUNC(MakeRect, make);
    /*! @brief Checks if a Point is inside the bounds of a Rect */
    FUNC(InsidePFunction, inside);
    /*! @brief Checks if to Rects intersect each other */
    FUNC(IntersectPFunction, intersect);
  } // namespace Function

  /*! @brief RectAdapter - A Lisple HostObject Adapter for Rect */
  NATIVE_ADAPTER(RectAdapter, Rect, (x, y, w, h), (x, y, w, h));

  class RectNamespace : public Lisple::Namespace
  {
   public:
    RectNamespace();
  };

} // namespace Pixils::Script

#endif /* RECT_NAMESPACE_H */
