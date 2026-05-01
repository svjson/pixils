#include "pixils/ui/view_update.h"

#include "pixils/runtime/hook_arguments.h"
#include "pixils/runtime/state.h"
#include "pixils/runtime/view.h"
#include "pixils/ui/view_events.h"
#include <pixils/hook_context.h>

#include <lisple/context.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/value.h>

namespace Pixils::UI
{
  namespace
  {
    Lisple::sptr_rtval invoke(const Lisple::sptr_rtval& fn,
                              Lisple::sptr_rtval_v& args,
                              Lisple::Runtime& rt,
                              const Lisple::sptr_rtval& fallback = Lisple::Constant::NIL)
    {
      if (!fn || fn->type == Lisple::RTValue::Type::NIL) return fallback;
      Lisple::Context exec_ctx(rt);
      return fn->exec().execute(exec_ctx, args);
    }

    void run_update_hook(const std::shared_ptr<Runtime::View>& view,
                         Runtime::HookArguments& hook_args,
                         Lisple::Runtime& rt)
    {
      Lisple::obj<HookContext>(*hook_args.update_args[1]).current_view = view;
      Lisple::sptr_rtval_v args = {view->state, hook_args.update_args[1]};
      view->state = invoke(view->mode->update, args, rt, view->state);
    }

    void bubble_child_events_to_subject(Runtime::View& subject,
                                        Lisple::sptr_rtval* subject_parent_state,
                                        const std::shared_ptr<Runtime::View>& child,
                                        Lisple::sptr_rtval& view_ctx,
                                        Lisple::Runtime& rt)
    {
      std::vector<CustomEvent> emitted_events;
      child->drain_events(emitted_events);
      emitted_events =
        process_view_events(subject, subject_parent_state, view_ctx, emitted_events, rt);

      for (auto& event : emitted_events)
      {
        subject.emitted_events.push_back(event);
      }
    }

    void update_interaction(Runtime::View& view,
                            const Point& mouse_pos,
                            const MouseState& mouse_state)
    {
      int mx = mouse_pos.round_x();
      int my = mouse_pos.round_y();

      view.interaction.hovered = view.bounds.w > 0 && mx >= view.bounds.x &&
                                 mx < view.bounds.x + view.bounds.w && my >= view.bounds.y &&
                                 my < view.bounds.y + view.bounds.h;

      view.interaction.pressed.clear();
      for (auto& [btn, chain] : mouse_state.button_chains)
      {
        for (auto& weak_v : chain)
        {
          if (auto v = weak_v.lock(); v && v.get() == &view)
          {
            view.interaction.pressed.insert(btn);
            break;
          }
        }
      }
    }

    void update_view_subtree(const std::shared_ptr<Runtime::View>& view_ptr,
                             const MouseState& mouse_state,
                             Lisple::sptr_rtval* parent_state,
                             const Point& mouse_pos,
                             Runtime::HookArguments& hook_args,
                             Lisple::Runtime& rt)
    {
      auto& view = *view_ptr;

      if (parent_state)
      {
        view.state = Runtime::extract_state(*parent_state, view);
      }

      update_interaction(view, mouse_pos, mouse_state);
      run_update_hook(view_ptr, hook_args, rt);

      for (auto& child : view.children)
      {
        update_view_subtree(child, mouse_state, &view.state, mouse_pos, hook_args, rt);
        bubble_child_events_to_subject(view,
                                       parent_state,
                                       child,
                                       hook_args.update_args[1],
                                       rt);
      }

      if (parent_state)
      {
        *parent_state = Runtime::merge_state(*parent_state, view, view.state);
      }
    }

  } // namespace

  void update_view_tree(const std::shared_ptr<Runtime::View>& root,
                        const MouseState& mouse_state,
                        const Point& mouse_pos,
                        Runtime::HookArguments& hook_args,
                        Lisple::Runtime& runtime)
  {
    update_view_subtree(root, mouse_state, nullptr, mouse_pos, hook_args, runtime);
  }

} // namespace Pixils::UI
