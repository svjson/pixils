#ifndef PIXILS__RUNTIME__COMPONENT_H
#define PIXILS__RUNTIME__COMPONENT_H

#include <pixils/runtime/view_definition.h>

#include <roo/runtime/value.h>
#include <string>
#include <vector>

namespace Pixils::Runtime
{
  struct UIStateModel
  {
    std::vector<std::string> owned_keys;
    std::vector<std::string> dependency_keys;
    Roo::sptr_val transition = Roo::Constant::NIL;
    Roo::sptr_val change_event = Roo::Constant::NIL;
  };

  struct Component : ViewDefinition
  {
    Roo::sptr_val init_ui = Roo::Constant::NIL;
    Roo::sptr_val update_ui = Roo::Constant::NIL;
    Roo::sptr_val after_layout_ui = Roo::Constant::NIL;
    std::vector<std::string> ui_state_keys;
    std::vector<UIStateModel> ui_state_models;
  };

} // namespace Pixils::Runtime

#endif /* PIXILS__RUNTIME__COMPONENT_H */
