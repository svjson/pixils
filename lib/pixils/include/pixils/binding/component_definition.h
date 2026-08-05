#ifndef PIXILS__BINDING__COMPONENT_DEFINITION_H
#define PIXILS__BINDING__COMPONENT_DEFINITION_H

#include <pixils/runtime/component.h>
#include <pixils/runtime/view_definition.h>

#include <roo/runtime/value.h>
#include <string>

namespace Roo
{
  class Context;
}

namespace Pixils::Script
{
  Runtime::Component build_component_from_definition(Roo::Context& ctx,
                                                     const Roo::sptr_val& definition_map);

  void reject_component_definition_fields(const Roo::sptr_val& definition_map,
                                          const std::string& context);

  bool is_view_definition_registry_value(const Roo::sptr_val& value);
  Runtime::ViewDefinition& definition_from_registry_value(const Roo::sptr_val& value,
                                                          const std::string& context);
  Runtime::Component* component_from_registry_value(const Roo::sptr_val& value);

} // namespace Pixils::Script

#endif /* PIXILS__BINDING__COMPONENT_DEFINITION_H */
