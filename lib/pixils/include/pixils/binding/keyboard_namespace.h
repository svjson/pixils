#ifndef PIXILS__KEYBOARD_NAMESPACE_H
#define PIXILS__KEYBOARD_NAMESPACE_H

#include <roo/exec.h>
#include <roo/namespace.h>

namespace Pixils::Script
{
  inline constexpr std::string_view NS__PIXILS__KEYBOARD = "pixils.keyboard";

  namespace Function
  {
    FUNC(EventToText, event_to_text);
  } // namespace Function

  class KeyboardNamespace : public Roo::Namespace
  {
   public:
    KeyboardNamespace();
  };
} // namespace Pixils::Script

#endif /* PIXILS__KEYBOARD_NAMESPACE_H */
