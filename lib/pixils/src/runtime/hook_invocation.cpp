#include "pixils/runtime/hook_invocation.h"

#include "pixils/hook_context.h"
#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_lifecycle.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exception.h>
#include <roo/exec.h>
#include <roo/host/object.h>
#include <roo/runtime.h>
#include <roo/runtime/value.h>
#include <unordered_map>

namespace Pixils::Runtime
{
  namespace
  {
    bool has_pending_child_mutations(const std::shared_ptr<View>& view)
    {
      return !view->pending_child_mutations.empty();
    }

    auto find_child(View& view, const std::string& child_id)
    {
      return std::find_if(view.children.begin(),
                          view.children.end(),
                          [&](const std::shared_ptr<View>& child)
                          { return child && child->id == child_id; });
    }

    std::shared_ptr<View> initialize_child(Roo::Runtime& runtime,
                                           View& parent,
                                           Asset::Registry& assets,
                                           const Roo::sptr_val& hook_ctx,
                                           const Roo::sptr_val& modes,
                                           const Roo::sptr_val& components,
                                           const ChildSlot& slot,
                                           Roo::sptr_val& parent_state)
    {
      auto child = UI::build_view_tree(slot, modes, components, runtime);
      UI::attach_style_view_tree(child, &parent);
      auto previous_parent_state = parent_state;
      parent_state = UI::init_view_tree(assets, runtime, hook_ctx, child, parent_state);
      parent.sync_ui_state_from_child_bindings(previous_parent_state, parent_state, runtime);
      parent.set_ui_state_from_child_bindings(*child, runtime);
      return child;
    }

    bool definition_matches(const View& child, const ChildSlot& slot)
    {
      return child.source_mode_name == slot.mode_name &&
             child.source_component_name == slot.component_name;
    }

    bool state_values_equal(const Roo::sptr_val& lhs, const Roo::sptr_val& rhs)
    {
      if (lhs == rhs) return true;
      if (!lhs || !rhs || lhs->type != rhs->type) return false;
      return *lhs == *rhs;
    }

    void apply_child_state_declaration(View& child, const ChildSlot& slot)
    {
      auto initial_state_changed =
        !state_values_equal(child.initial_state, slot.initial_state);

      child.state_binding = slot.state_binding;
      child.component_state_binding = slot.component_state_binding;
      child.ui_state_binding = slot.ui_state_binding;
      child.initial_state = slot.initial_state;
      child.state_policy = slot.state_policy.value_or("shared");

      if (initial_state_changed)
      {
        child.set_state_if_changed(slot.initial_state);
      }
    }

    bool same_children(const std::vector<std::shared_ptr<View>>& current,
                       const std::vector<std::shared_ptr<View>>& desired)
    {
      return current.size() == desired.size() &&
             std::equal(current.begin(), current.end(), desired.begin());
    }

    void reconcile_children(Roo::Runtime& runtime,
                            View& parent,
                            Asset::Registry& assets,
                            const Roo::sptr_val& hook_ctx,
                            const Roo::sptr_val& modes,
                            const Roo::sptr_val& components,
                            const std::vector<ChildSlot>& slots,
                            Roo::sptr_val& parent_state)
    {
      std::unordered_map<std::string, std::shared_ptr<View>> current;
      for (const auto& child : parent.children)
      {
        if (child) current.emplace(child->id, child);
      }

      std::vector<std::shared_ptr<View>> desired;
      desired.reserve(slots.size());
      for (const auto& slot : slots)
      {
        auto existing = current.find(slot.id);
        if (existing == current.end())
        {
          desired.push_back(initialize_child(runtime,
                                             parent,
                                             assets,
                                             hook_ctx,
                                             modes,
                                             components,
                                             slot,
                                             parent_state));
          continue;
        }

        if (!definition_matches(*existing->second, slot))
        {
          throw Roo::InvocationException(
            "ui/reconcile-children! cannot change the mode or component of child '" +
            slot.id + "'; use ui/replace-child! explicitly");
        }

        apply_child_state_declaration(*existing->second, slot);
        desired.push_back(existing->second);
        current.erase(existing);
      }

      if (!same_children(parent.children, desired))
      {
        parent.children = std::move(desired);
        parent.mark_children_changed();
      }
    }

  } // namespace

  Roo::sptr_val apply_pending_child_mutations(Roo::Runtime& runtime,
                                              const std::shared_ptr<View>& view,
                                              const Roo::sptr_val& hook_ctx,
                                              const Roo::sptr_val& base_state)
  {
    if (!has_pending_child_mutations(view))
    {
      return base_state;
    }

    auto modes = runtime.lookup(Script::ID__PIXILS__MODES);
    auto components = runtime.lookup(Script::ID__PIXILS__COMPONENTS);
    auto& render_ctx =
      Roo::obj<RenderContext>(*runtime.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
    auto* assets = render_ctx.asset_registry.get();
    if (!assets)
    {
      return base_state;
    }

    auto parent_state = base_state;
    auto mutations = std::move(view->pending_child_mutations);
    view->pending_child_mutations.clear();

    for (auto& mutation : mutations)
    {
      switch (mutation.type)
      {
      case ChildMutationType::REMOVE:
      {
        auto child_it = find_child(*view, mutation.child_id);
        if (child_it == view->children.end()) break;

        view->children.erase(child_it);
        view->mark_children_changed();
        break;
      }

      case ChildMutationType::REPLACE:
      {
        auto child_it = find_child(*view, mutation.child_id);
        if (child_it == view->children.end()) break;

        *child_it = initialize_child(runtime,
                                     *view,
                                     *assets,
                                     hook_ctx,
                                     modes,
                                     components,
                                     mutation.child_slot,
                                     parent_state);
        view->mark_children_changed();
        break;
      }

      case ChildMutationType::APPEND:
        view->children.push_back(initialize_child(runtime,
                                                  *view,
                                                  *assets,
                                                  hook_ctx,
                                                  modes,
                                                  components,
                                                  mutation.child_slot,
                                                  parent_state));
        view->mark_children_changed();
        break;

      case ChildMutationType::RECONCILE:
        reconcile_children(runtime,
                           *view,
                           *assets,
                           hook_ctx,
                           modes,
                           components,
                           mutation.child_slots,
                           parent_state);
        break;
      }
    }

    for (auto& child : view->children)
    {
      UI::restore_view_tree(child, parent_state);
    }

    return parent_state;
  }

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
    auto next_state = (result && result->type != Roo::Value::Type::NIL) ? result : fallback;
    return apply_pending_child_mutations(runtime, view, args.back(), next_state);
  }
} // namespace Pixils::Runtime
