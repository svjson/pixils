
#ifndef PIXILS__BINDING__UI_NAMESPACE_H
#define PIXILS__BINDING__UI_NAMESPACE_H

#include <pixils/ui/event.h>

#include <lisple/exec.h>
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
  inline constexpr std::string_view FN__PIXILS__UI__BLUR_BANG = "blur!";
  inline constexpr std::string_view FN__PIXILS__UI__CHILDREN = "children";
  inline constexpr std::string_view FN__PIXILS__UI__FOCUS_BANG = "focus!";
  inline constexpr std::string_view FN__PIXILS__UI__STYLE_BANG = "style!";
  inline constexpr std::string_view FN__PIXILS__UI__THEME_VAR = "theme-var";

  namespace Function
  {
    FUNC(BindStateFn, bind_state);
    FUNC(BlurBangFunction, blur);
    FUNC(ChildrenFunction, children);
    FUNC(EmitBangFunction, emit);
    FUNC(FocusBangFunction, focus);
    FUNC(ReplaceChildBangFunction, replace_child);
    FUNC(StyleBangFunction, style);
    FUNC(StopPropagation, stop);
    FUNC(ActiveThemeVarFunction, theme_var);
  } // namespace Function

  NATIVE_ADAPTER(EventAdapter, Event);
  NATIVE_SUB_ADAPTER(EventAdapter,
                     (CustomEventAdapter, CustomEvent),
                     (event_key, source_mode, payload));
  NATIVE_SUB_ADAPTER(EventAdapter,
                     (MouseEventAdapter, MouseEvent),
                     (global_pos, local_pos));
  NATIVE_SUB_ADAPTER(MouseEventAdapter,
                     (MouseButtonEventAdapter, MouseButtonEvent),
                     (button, click_count));
  NATIVE_SUB_ADAPTER(MouseButtonEventAdapter,
                     (DragEventAdapter, DragEvent),
                     (start_global_pos, start_local_pos, delta, total_delta, payload));
  NATIVE_SUB_ADAPTER(EventAdapter,
                     (KeyboardEventAdapter, KeyboardEvent),
                     (key, held_keys, match));
  NATIVE_ADAPTER(BindStateAdapter, Runtime::BindState);

  class UINamespace : public Lisple::Namespace
  {
   public:
    UINamespace();
  };

} // namespace Pixils::Script

#endif /* PIXILS__BINDING__UI_NAMESPACE_H */
