#include "pixils/ui/view_update.h"

#include "pixils/runtime/hook_arguments.h"
#include "pixils/runtime/hook_invocation.h"
#include "pixils/runtime/state.h"
#include "pixils/runtime/view.h"
#include "pixils/ui/view_events.h"
#include <pixils/hook_context.h>

#include <roo/host/object.h>
#include <roo/runtime.h>
#include <roo/runtime/value.h>
#include <utility>

namespace Pixils::UI
{
  namespace
  {
    bool has_hook(const Roo::sptr_val& hook)
    {
      return hook && hook->type != Roo::Value::Type::NIL;
    }

    bool has_state_binding(const Runtime::View& view)
    {
      return view.state_binding && view.state_binding->type != Roo::Value::Type::NIL;
    }

    void run_update_hook(const std::shared_ptr<Runtime::View>& view,
                         Runtime::HookArguments& hook_args,
                         Roo::Runtime& rt)
    {
      if (!has_hook(view->mode->update)) return;

      Roo::obj<HookContext>(*hook_args.update_args[1]).current_view = view;
      Roo::sptr_val_v args = {view->state, hook_args.update_args[1]};
      view->set_state_if_changed(
        Runtime::invoke_hook(rt, view, view->mode->update, args, view->state));
    }

    void bubble_child_events_to_subject(Runtime::View& subject,
                                        Roo::sptr_val* subject_parent_state,
                                        Runtime::View* subject_parent_view,
                                        const std::shared_ptr<Runtime::View>& child,
                                        Roo::sptr_val& view_ctx,
                                        Roo::Runtime& rt)
    {
      std::vector<CustomEvent> emitted_events;
      child->drain_events(emitted_events);
      emitted_events = process_view_events(subject,
                                           subject_parent_state,
                                           subject_parent_view,
                                           view_ctx,
                                           emitted_events,
                                           rt);

      for (auto& event : emitted_events)
      {
        subject.emitted_events.push_back(event);
      }
    }

    void update_interaction(Runtime::View& view,
                            const Point&,
                            const MouseState& mouse_state,
                            const FocusState& focus_state)
    {
      auto next_interaction = view.interaction;
      next_interaction.hovered = false;
      for (auto& weak_v : mouse_state.hovered_chain)
      {
        if (auto v = weak_v.lock(); v && v.get() == &view)
        {
          next_interaction.hovered = true;
          break;
        }
      }
      next_interaction.focused = false;
      next_interaction.focus_within = false;

      next_interaction.pressed.clear();
      for (auto& [btn, chain] : mouse_state.button_chains)
      {
        for (auto& weak_v : chain)
        {
          if (auto v = weak_v.lock(); v && v.get() == &view)
          {
            next_interaction.pressed.insert(btn);
            break;
          }
        }
      }

      auto focused_view = focus_state.focused.lock();
      if (focused_view && focused_view.get() == &view)
      {
        next_interaction.focused = true;
      }

      for (auto& weak_v : focus_state.focus_chain)
      {
        if (auto v = weak_v.lock(); v && v.get() == &view)
        {
          next_interaction.focus_within = true;
          break;
        }
      }

      if (view.interaction.hovered != next_interaction.hovered ||
          view.interaction.focused != next_interaction.focused ||
          view.interaction.focus_within != next_interaction.focus_within ||
          view.interaction.pressed != next_interaction.pressed)
      {
        view.interaction = std::move(next_interaction);
        view.mark_interaction_changed();
      }
    }

    void update_view_subtree(const std::shared_ptr<Runtime::View>& view_ptr,
                             const MouseState& mouse_state,
                             const FocusState& focus_state,
                             Roo::sptr_val* parent_state,
                             Runtime::View* parent_view,
                             const Point& mouse_pos,
                             Runtime::HookArguments& hook_args,
                             Roo::Runtime& rt)
    {
      auto& view = *view_ptr;

      if (parent_state && has_state_binding(view))
      {
        view.set_state_if_changed(Runtime::extract_state(*parent_state, view));
      }

      update_interaction(view, mouse_pos, mouse_state, focus_state);
      run_update_hook(view_ptr, hook_args, rt);

      for (auto& child : view.children)
      {
        update_view_subtree(child,
                            mouse_state,
                            focus_state,
                            &view.state,
                            &view,
                            mouse_pos,
                            hook_args,
                            rt);
        bubble_child_events_to_subject(view,
                                       parent_state,
                                       parent_view,
                                       child,
                                       hook_args.update_args[1],
                                       rt);
      }

      if (parent_state && has_state_binding(view))
      {
        auto merged = Runtime::merge_state(*parent_state, view, view.state);
        if (parent_view)
        {
          parent_view->set_state_if_changed(merged);
        }
        else
        {
          *parent_state = merged;
        }
      }
    }

    void refresh_interaction_subtree(const std::shared_ptr<Runtime::View>& view_ptr,
                                     const MouseState& mouse_state,
                                     const FocusState& focus_state,
                                     const Point& mouse_pos)
    {
      if (!view_ptr)
      {
        return;
      }

      auto& view = *view_ptr;
      update_interaction(view, mouse_pos, mouse_state, focus_state);

      for (auto& child : view.children)
      {
        refresh_interaction_subtree(child, mouse_state, focus_state, mouse_pos);
      }
    }

  } // namespace

  void update_view_tree(const std::shared_ptr<Runtime::View>& root,
                        const MouseState& mouse_state,
                        const FocusState& focus_state,
                        const Point& mouse_pos,
                        Runtime::HookArguments& hook_args,
                        Roo::Runtime& runtime)
  {
    update_view_subtree(root,
                        mouse_state,
                        focus_state,
                        nullptr,
                        nullptr,
                        mouse_pos,
                        hook_args,
                        runtime);
  }

  void refresh_view_interaction_tree(const std::shared_ptr<Runtime::View>& root,
                                     const MouseState& mouse_state,
                                     const FocusState& focus_state,
                                     const Point& mouse_pos)
  {
    refresh_interaction_subtree(root, mouse_state, focus_state, mouse_pos);
  }

} // namespace Pixils::UI
