
#ifndef PIXILS__BINDING__UI_NAMESPACE_H
#define PIXILS__BINDING__UI_NAMESPACE_H

#include <pixils/ui/event.h>

#include <lisple/exec.h>
#include <lisple/host.h>
#include <lisple/host/object.h>
#include <lisple/namespace.h>

namespace Lisple
{
  class Runtime;
}

namespace Pixils::Runtime
{
  struct BindState;
}

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__UI = "pixils.ui";

  namespace Function
  {
    FUNC(BindStateFn, bind_state);
    FUNC(EmitBangFunction, emit);
    FUNC(ReplaceChildBangFunction, replace_child);
    FUNC(StopPropagation, stop);
  } // namespace Function

  NATIVE_ADAPTER(CustomEventAdapter, CustomEvent, (event_key, source_mode, payload));
  NATIVE_ADAPTER(MouseEventAdapter, MouseEvent, (global_pos, local_pos));
  NATIVE_SUB_ADAPTER(MouseEventAdapter,
                     (MouseButtonEventAdapter, MouseButtonEvent),
                     (button));
  NATIVE_SUB_ADAPTER(MouseButtonEventAdapter,
                     (DragEventAdapter, DragEvent),
                     (start_global_pos, start_local_pos, delta, total_delta));
  NATIVE_ADAPTER(KeyboardEventAdapter, KeyboardEvent, (key, held_keys, match));
  NATIVE_ADAPTER(BindStateAdapter, Runtime::BindState);

  class UINamespace : public Lisple::Namespace
  {
   public:
    UINamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__BINDING__UI_NAMESPACE_H */
