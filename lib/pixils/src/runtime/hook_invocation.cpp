#include "pixils/runtime/hook_invocation.h"

#include "pixils/hook_context.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_lifecycle.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exec.h>
#include <roo/host/object.h>
#include <roo/runtime.h>
#include <roo/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    Roo::sptr_val apply_pending_child_replacements(Roo::Runtime& runtime,
                                                      const std::shared_ptr<View>& view,
                                                      const Roo::sptr_val& hook_ctx,
                                                      const Roo::sptr_val& base_state)
    {
      if (view->pending_child_replacements.empty())
      {
        return base_state;
      }

      auto modes = runtime.lookup(Script::ID__PIXILS__MODES);
      auto& render_ctx =
        Roo::obj<RenderContext>(*runtime.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
      auto* assets = render_ctx.asset_registry.get();
      if (!assets)
      {
        return base_state;
      }

      auto parent_state = base_state;
      auto replacements = std::move(view->pending_child_replacements);
      view->pending_child_replacements.clear();

      for (auto& replacement : replacements)
      {
        auto child_it = std::find_if(view->children.begin(),
                                     view->children.end(),
                                     [&](const std::shared_ptr<View>& child)
                                     { return child && child->id == replacement.child_id; });
        if (child_it == view->children.end())
        {
          continue;
        }

        auto new_child = UI::build_view_tree(replacement.child_slot, modes, runtime);
        UI::attach_style_view_tree(new_child, view.get());
        parent_state =
          UI::init_view_tree(*assets, runtime, hook_ctx, new_child, parent_state);
        *child_it = std::move(new_child);
        view->mark_children_changed();
      }

      for (auto& child : view->children)
      {
        UI::restore_view_tree(child, parent_state);
      }

      return parent_state;
    }
  } // namespace

  Roo::sptr_val invoke_hook(Roo::Runtime& runtime,
                               const std::shared_ptr<View>& view,
                               const Roo::sptr_val& fn,
                               Roo::sptr_val_v& args,
                               const Roo::sptr_val& fallback)
  {
    if (!fn || fn->type == Roo::Value::Type::NIL) return fallback;

    Roo::obj<HookContext>(*args.back()).current_view = view;
    Roo::Context exec_ctx(runtime);
    auto result = fn->exec().execute(exec_ctx, args);
    auto next_state =
      (result && result->type != Roo::Value::Type::NIL) ? result : fallback;
    return apply_pending_child_replacements(runtime, view, args.back(), next_state);
  }
} // namespace Pixils::Runtime
