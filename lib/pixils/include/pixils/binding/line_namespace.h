#ifndef PIXILS__LINE_NAMESPACE_H
#define PIXILS__LINE_NAMESPACE_H

#include <pixils/geom.h>

#include <roo/exec.h>
#include <roo/namespace.h>
#include <string>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__LINE = "pixils.line";

  namespace Function
  {
    /*! @brief Return the closest point on finite line a-b to a point */
    FUNC(LineClosestPointFunction, closest_point);
  } // namespace Function

  class LineNamespace : public Roo::Namespace
  {
   public:
    LineNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__LINE_NAMESPACE_H */
