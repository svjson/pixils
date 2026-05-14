#ifndef PIXILS__UI__CURSOR_H
#define PIXILS__UI__CURSOR_H

#include <pixils/geom.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace Pixils::UI
{
  enum class SystemCursor : uint8_t
  {
    DEFAULT,
    POINTER,
    TEXT,
    CROSSHAIR,
    MOVE,
    NOT_ALLOWED,
    WAIT,
    PROGRESS,
    RESIZE_X,
    RESIZE_Y,
    RESIZE_NWSE,
    RESIZE_NESW,
  };

  struct ImageCursor
  {
    enum class RenderMode : uint8_t
    {
      APP,
      NATIVE,
    };

    std::optional<std::pair<std::string, std::string>> image;
    std::optional<Rect> source;
    Point hotspot{0, 0};
    int scale = 1;
    RenderMode render_mode = RenderMode::APP;

    bool operator==(const ImageCursor& other) const;
  };

  struct CursorSpec
  {
    enum class Kind : uint8_t
    {
      SYSTEM,
      NAMED,
      IMAGE,
    };

    Kind kind = Kind::SYSTEM;
    SystemCursor system = SystemCursor::DEFAULT;
    std::string name;
    ImageCursor image;

    static CursorSpec system_cursor(SystemCursor cursor);
    static CursorSpec named(std::string name);
    static CursorSpec image_cursor(ImageCursor image);

    bool operator==(const CursorSpec& other) const;
  };
} // namespace Pixils::UI

#endif /* PIXILS__UI__CURSOR_H */
