#ifndef PIXILS__UI__VIEW_LIFECYCLE_H
#define PIXILS__UI__VIEW_LIFECYCLE_H

#include <lisple/runtime/value.h>

#include <memory>

namespace Pixils::Runtime
{
  struct ChildSlot;
  struct Mode;
  struct Session;
  struct View;
}

namespace Lisple
{
  class Runtime;
}

namespace Pixils::UI
{
  void attach_view_mode(Runtime::View& view,
                        Runtime::Mode& base_mode,
                        const Lisple::sptr_rtval& overrides,
                        Lisple::Runtime& runtime);

  std::shared_ptr<Runtime::View> build_view_tree(const Runtime::ChildSlot& slot,
                                                 const Lisple::sptr_rtval& modes,
                                                 Lisple::Runtime& runtime);

  Lisple::sptr_rtval init_view_tree(Runtime::Session& session,
                                    const std::shared_ptr<Runtime::View>& view,
                                    const Lisple::sptr_rtval& parent_state);

  void restore_view_tree(const std::shared_ptr<Runtime::View>& view,
                         const Lisple::sptr_rtval& parent_state);

} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_LIFECYCLE_H */
