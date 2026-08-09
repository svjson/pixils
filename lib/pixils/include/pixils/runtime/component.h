#ifndef PIXILS__RUNTIME__COMPONENT_H
#define PIXILS__RUNTIME__COMPONENT_H

#include <pixils/runtime/view_definition.h>

#include <roo/runtime/value.h>
#include <string>
#include <vector>

namespace Pixils::Runtime
{
  struct Component : ViewDefinition
  {
    Roo::sptr_val init_ui = Roo::Constant::NIL;
    Roo::sptr_val update_ui = Roo::Constant::NIL;
    Roo::sptr_val after_layout_ui = Roo::Constant::NIL;
    std::vector<std::string> ui_state_keys;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__COMPONENT_H */
