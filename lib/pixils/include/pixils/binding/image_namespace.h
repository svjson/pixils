#ifndef PIXILS__IMAGE_NAMESPACE_H
#define PIXILS__IMAGE_NAMESPACE_H

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__IMAGE = "pixils.image";

  inline constexpr std::string_view FN__HEIGHT = "height";
  inline constexpr std::string_view FN__RECT = "rect";
  inline constexpr std::string_view FN__SIZE = "size";
  inline constexpr std::string_view FN__WIDTH = "width";

  namespace Function
  {
    FUNC(ImageHeight, height);
    FUNC(ImageRect, rect, rect_with_offset);
    FUNC(ImageSize, size);
    FUNC(ImageWidth, width);
  } // namespace Function

  class ImageNamespace : public Lisple::Namespace
  {
   public:
    ImageNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__IMAGE_NAMESPACE_H */
