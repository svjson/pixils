#include "pixils/binding/keyboard_namespace.h"

#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/keyboard.h>
#include <pixils/ui/event.h>

#include <roo/host/object.h>
#include <roo/runtime/value.h>

namespace Pixils::Script
{
  namespace Function
  {
    FUNC_IMPL(EventToText,
              SIG((FN_ARGS((&HostType::KEYBOARD_EVENT)),
                   EXEC_DISPATCH(&EventToText::exec_event_to_text))));

    EXEC_BODY(EventToText, exec_event_to_text)
    {
      auto& event = Roo::obj<KeyboardEvent>(*args[0]);
      auto text = Keyboard::key_to_text(event.key, event.held_keys);
      if (!text)
      {
        return Roo::Constant::NIL;
      }

      return Roo::string(*text);
    }
  } // namespace Function

  KeyboardNamespace::KeyboardNamespace()
    : Roo::Namespace(std::string(NS__PIXILS__KEYBOARD))
  {
    values.emplace("event->text", Function::EventToText::make());
  }
} // namespace Pixils::Script
