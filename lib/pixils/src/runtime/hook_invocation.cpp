#include "pixils/runtime/hook_invocation.h"

#include "pixils/hook_context.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_lifecycle.h>

#include <algorithm>
#include <lisple/context.h>
#include <lisple/exec.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    Lisple::sptr_val apply_pending_child_replacements(Lisple::Runtime& runtime,
                                                      const std::shared_ptr<View>& view,
                                                      const Lisple::sptr_val& hook_ctx,
                                                      const Lisple::sptr_val& base_state)
    {
      if (view->pending_child_replacements.empty())
      {
        return base_state;
      }

      auto modes = runtime.lookup(Script::ID__PIXILS__MODES);
      auto& render_ctx =
        Lisple::obj<RenderContext>(*runtime.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
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
        parent_state =
          UI::init_view_tree(*assets, runtime, hook_ctx, new_child, parent_state);
        *child_it = std::move(new_child);
      }

      for (auto& child : view->children)
      {
        UI::restore_view_tree(child, parent_state);
      }

      return parent_state;
    }
  } // namespace

  Lisple::sptr_val invoke_hook(Lisple::Runtime& runtime,
                               const std::shared_ptr<View>& view,
                               const Lisple::sptr_val& fn,
                               Lisple::sptr_val_v& args,
                               const Lisple::sptr_val& fallback)
  {
    if (!fn || fn->type == Lisple::Value::Type::NIL) return fallback;

    Lisple::obj<HookContext>(*args.back()).current_view = view;
    Lisple::Context exec_ctx(runtime);
    auto result = fn->exec().execute(exec_ctx, args);
    auto next_state =
      (result && result->type != Lisple::Value::Type::NIL) ? result : fallback;
    return apply_pending_child_replacements(runtime, view, args.back(), next_state);
  }
} // namespace Pixils::Runtime
