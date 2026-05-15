#ifndef PIXILS__UI__DRAG_H
#define PIXILS__UI__DRAG_H

#include <pixils/geom.h>
#include <pixils/ui/mouse_button.h>

#include <lisple/form.h>
#include <memory>

namespace Pixils::Runtime
{
  struct View;
}

namespace Pixils::UI
{
  enum class DragStartMode
  {
    MOTION,
    IMMEDIATE,
    THRESHOLD,
  };

  struct DragStartPolicy
  {
    DragStartMode mode = DragStartMode::MOTION;
    int distance = 1;
  };

  struct DragPolicy
  {
    MouseButton button = MouseButton::LEFT;
    DragStartPolicy start;
    Lisple::sptr_val payload = Lisple::Constant::NIL;
  };

  struct DragOperation
  {
    MouseButton button = MouseButton::NONE;
    std::weak_ptr<Runtime::View> source;
    Point start_global_pos;
    Point current_global_pos;
    Lisple::sptr_val payload = Lisple::Constant::NIL;
    DragPolicy policy;
  };

} // namespace Pixils::UI

#endif /* PIXILS__UI__DRAG_H */
