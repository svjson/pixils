#ifndef PIXILS__UI__VIEW_LIFECYCLE_H
#define PIXILS__UI__VIEW_LIFECYCLE_H

#include <roo/form.h>
#include <roo/runtime/value.h>
#include <memory>

namespace Pixils::Asset
{
  class Registry;
}

namespace Pixils::Runtime
{
  struct ChildSlot;
  struct Mode;
  struct View;
} // namespace Pixils::Runtime

namespace Roo
{
  class Runtime;
}

namespace Pixils::UI
{
  std::shared_ptr<Runtime::View> build_root_view(Runtime::Mode& base_mode,
                                                 const Roo::sptr_val& state,
                                                 const Roo::sptr_val& overrides,
                                                 Roo::Runtime& runtime);

  void attach_view_mode(Runtime::View& view,
                        Runtime::Mode& base_mode,
                        const Roo::sptr_val& overrides,
                        Roo::Runtime& runtime);

  std::shared_ptr<Runtime::View> build_view_tree(const Runtime::ChildSlot& slot,
                                                 const Roo::sptr_val& modes,
                                                 Roo::Runtime& runtime);

  void attach_style_view_tree(const std::shared_ptr<Runtime::View>& view,
                              Runtime::View* parent);

  Roo::sptr_val init_view_tree(Asset::Registry& assets,
                                  Roo::Runtime& runtime,
                                  const Roo::sptr_val& init_hook_ctx,
                                  const std::shared_ptr<Runtime::View>& view,
                                  const Roo::sptr_val& parent_state);

  void init_root_view(Asset::Registry& assets,
                      Roo::Runtime& runtime,
                      const Roo::sptr_val& init_hook_ctx,
                      const std::shared_ptr<Runtime::View>& view);

  void restore_view_tree(const std::shared_ptr<Runtime::View>& view,
                         const Roo::sptr_val& parent_state);

} // namespace Pixils::UI

#endif /* PIXILS__UI__VIEW_LIFECYCLE_H */
