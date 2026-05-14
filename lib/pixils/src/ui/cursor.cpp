#include <pixils/ui/cursor.h>

#include <utility>

namespace Pixils::UI
{
  bool ImageCursor::operator==(const ImageCursor& other) const
  {
    return image == other.image && source == other.source && hotspot == other.hotspot &&
           scale == other.scale && render_mode == other.render_mode;
  }

  CursorSpec CursorSpec::system_cursor(SystemCursor cursor)
  {
    CursorSpec spec;
    spec.kind = Kind::SYSTEM;
    spec.system = cursor;
    return spec;
  }

  CursorSpec CursorSpec::named(std::string name)
  {
    CursorSpec spec;
    spec.kind = Kind::NAMED;
    spec.name = std::move(name);
    return spec;
  }

  CursorSpec CursorSpec::image_cursor(ImageCursor image)
  {
    CursorSpec spec;
    spec.kind = Kind::IMAGE;
    spec.image = std::move(image);
    return spec;
  }

  bool CursorSpec::operator==(const CursorSpec& other) const
  {
    return kind == other.kind && system == other.system && name == other.name &&
           image == other.image;
  }
} // namespace Pixils::UI
