#ifndef PIXILS__RUNTIME__STATE_BINDING_H
#define PIXILS__RUNTIME__STATE_BINDING_H

#include <roo/runtime/value.h>

namespace Pixils::Runtime::StateBindings
{
  bool is_binding(const Roo::sptr_val& value);
  bool contains_binding(const Roo::sptr_val& value);
  bool contains_readable_binding(const Roo::sptr_val& value);
  Roo::sptr_val literal_value(const Roo::sptr_val& value);
  Roo::sptr_val resolve_value(const Roo::sptr_val& parent,
                              const Roo::sptr_val& current,
                              const Roo::sptr_val& binding);
  Roo::sptr_val merge_pass(Roo::sptr_val result,
                           const Roo::sptr_val& binding,
                           const Roo::sptr_val& child_state,
                           bool whole_parent_bindings,
                           bool child_value_present = true);
} // namespace Pixils::Runtime::StateBindings

#endif /* PIXILS__RUNTIME__STATE_BINDING_H */
