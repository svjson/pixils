#include <pixils/benchmark/counters.h>
#include <pixils/binding/ui/ui_namespace.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/session.h>
#include <pixils/runtime/view.h>
#include <pixils/runtime/view_state_binding.h>
#include <pixils/ui/event.h>
#include <pixils/ui/interaction_dispatch.h>
#include <pixils/ui/view_events.h>
#include <pixils/ui/view_update.h>

#include <algorithm>
#include <optional>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    Point update_mouse_pos(const HookArguments& hook_args)
    {
      if (!hook_args.events || !hook_args.events->mouse_pos ||
          hook_args.events->mouse_pos->type == Roo::Value::Type::NIL)
      {
        return {0.0f, 0.0f};
      }

      return Roo::obj<Point>(*hook_args.events->mouse_pos);
    }

    bool has_pointer_transient(const FrameEvents& events)
    {
      return events.mouse_moved ||
             (events.mouse_button_down &&
              events.mouse_button_down->type != Roo::Value::Type::NIL) ||
             (events.mouse_button_up &&
              events.mouse_button_up->type != Roo::Value::Type::NIL) ||
             (events.mouse_wheel && events.mouse_wheel->type != Roo::Value::Type::NIL);
    }

    bool contains(const Rect& bounds, const Point& point)
    {
      return point.x >= bounds.x && point.x < bounds.x + bounds.w && point.y >= bounds.y &&
             point.y < bounds.y + bounds.h;
    }

    void push_scope_state_to_root(const std::shared_ptr<View>& scope, Roo::Runtime& runtime)
    {
      for (View* view = scope.get(); view && view->parent; view = view->parent)
      {
        push_bound_view_state(*view, runtime);
      }
    }

    bool belongs_to_root(const std::shared_ptr<View>& scope,
                         const std::shared_ptr<View>& root)
    {
      for (View* view = scope.get(); view; view = view->parent)
      {
        if (view == root.get()) return true;
      }
      return false;
    }

    struct ScopedInteractionTarget
    {
      Session::ModeFrameMetadata::ScopedComposition::InteractionScope* scope = nullptr;
      UI::FocusState* focus_state = nullptr;
      std::shared_ptr<View> owner_root;
      size_t mode_offset = 0;
      int pass_depth = 0;
    };

    std::optional<ScopedInteractionTarget> scoped_interaction_target(Session& session,
                                                                     const Point& point)
    {
      if (!session.active_mode || UI::interaction_hit_depth(session.active_mode, point) > 1)
      {
        return std::nullopt;
      }

      const size_t frame_count = session.mode_stack.size();
      if (frame_count <= 1 || session.frame_metadata.size() < frame_count ||
          session.ctx_stack.size() + 1 < frame_count)
      {
        return std::nullopt;
      }
      int pass_depth = 0;
      for (size_t child_frame = frame_count - 1; child_frame > 0; child_frame--)
      {
        auto& metadata = session.frame_metadata[child_frame];
        auto& scopes = metadata.scoped_composition.interaction_pass;
        if (scopes.empty()) break;

        pass_depth++;
        if (metadata.overlay && metadata.overlay->anchor_view &&
            contains(metadata.overlay->anchor_view->visual_bounds, point))
        {
          break;
        }

        for (auto& scope : scopes)
        {
          const size_t owner_frame = child_frame - 1;
          auto owner_root = session.ctx_stack[owner_frame];
          if (belongs_to_root(scope.view, owner_root) &&
              UI::interaction_hit_depth(scope.view, point) > 0)
          {
            return ScopedInteractionTarget{
              .scope = &scope,
              .focus_state = &metadata.restore_focus,
              .owner_root = std::move(owner_root),
              .mode_offset = frame_count - 1 - owner_frame,
              .pass_depth = pass_depth,
            };
          }
        }
      }

      return std::nullopt;
    }

    void update_scoped_passes(
      Session& session,
      const Point& mouse_pos,
      const std::optional<ScopedInteractionTarget>& interaction_target)
    {
      struct Target
      {
        std::shared_ptr<View> scope;
        UI::MouseState* mouse_state;
        UI::FocusState* focus_state;
        std::shared_ptr<View> owner_root;
        size_t mode_offset;
      };

      std::vector<Target> targets;
      const size_t frame_count = session.mode_stack.size();
      if (frame_count <= 1 || session.frame_metadata.size() < frame_count ||
          session.ctx_stack.size() + 1 < frame_count)
      {
        return;
      }
      for (size_t child_frame = frame_count - 1; child_frame > 0; child_frame--)
      {
        auto& metadata = session.frame_metadata[child_frame];
        auto& scopes = metadata.scoped_composition.update_pass;
        if (scopes.empty()) break;

        const size_t owner_frame = child_frame - 1;
        for (auto& scope : scopes)
        {
          auto owner_root = session.ctx_stack[owner_frame];
          if (!belongs_to_root(scope, owner_root)) continue;

          UI::MouseState* scope_mouse_state = nullptr;
          auto interaction_scope =
            std::find_if(metadata.scoped_composition.interaction_pass.begin(),
                         metadata.scoped_composition.interaction_pass.end(),
                         [&](const auto& candidate) { return candidate.view == scope; });
          if (interaction_scope != metadata.scoped_composition.interaction_pass.end())
          {
            scope_mouse_state = &interaction_scope->mouse_state;
          }
          targets.push_back({.scope = scope,
                             .mouse_state = scope_mouse_state,
                             .focus_state = &metadata.restore_focus,
                             .owner_root = std::move(owner_root),
                             .mode_offset = frame_count - 1 - owner_frame});
        }
      }

      UI::MouseState empty_mouse_state;
      for (auto it = targets.rbegin(); it != targets.rend(); ++it)
      {
        if (interaction_target && interaction_target->scope->view == it->scope)
        {
          continue;
        }
        UI::update_view_tree(it->scope,
                             it->mouse_state ? *it->mouse_state : empty_mouse_state,
                             *it->focus_state,
                             mouse_pos,
                             session.hook_args,
                             session.roo_runtime);
        push_scope_state_to_root(it->scope, session.roo_runtime);
        session.mode_stack.update_state(it->owner_root->state, it->mode_offset);
      }
    }

  } // namespace

  void Session::update_mode()
  {
    PIXILS_BENCHMARK_COUNT(runtime_update_mode_calls);
    PIXILS_BENCHMARK_TIME_BLOCK(runtime_update_time_ns);

    auto update_stack = mode_stack.get_update_stack();

    std::vector<CustomEvent> emitted_events;

    /**
     * Update composition modes below the top, preserving the existing offset semantics.
     */
    for (size_t i = update_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      auto view = ctx_stack[ctx_idx];
      View& ctx = *view;

      Roo::sptr_val_v rargs = this->hook_args.update_args;
      auto ctx_parent_state = ctx.state;
      emitted_events = UI::process_view_events(ctx,
                                               &ctx_parent_state,
                                               nullptr,
                                               rargs.back(),
                                               emitted_events,
                                               roo_runtime);
      rargs[0] = ctx.state;
      if (ctx.definition->update && ctx.definition->update->type != Roo::Value::Type::NIL)
      {
        ctx.set_state_if_changed(
          invoke_hook(roo_runtime, view, ctx.definition->update, rargs, ctx.state));
      }
      mode_stack.update_state(ctx.state, i);

      ctx.drain_events(emitted_events);
    }

    if (hook_args.events)
    {
      Pixils::UI::dispatch_keyboard_events(active_mode,
                                           focus_state,
                                           *hook_args.events,
                                           hook_args,
                                           roo_runtime);
    }

    /**
     * Delegate update, hover tracking, and event dispatch to UI functions.
     * Non-modal overlays can pass pointer interaction through to the mode below.
     */
    if (hook_args.events)
    {
      const Point mouse_pos = update_mouse_pos(hook_args);
      auto scoped_target = has_pointer_transient(*hook_args.events)
                             ? scoped_interaction_target(*this, mouse_pos)
                             : std::nullopt;
      update_scoped_passes(*this, mouse_pos, scoped_target);

      if (scoped_target)
      {
        FrameEvents passive_events = *hook_args.events;
        passive_events.clear_transients();
        Pixils::UI::dispatch_interactions(active_mode,
                                          mouse_state,
                                          focus_state,
                                          passive_events,
                                          hook_args,
                                          roo_runtime);

        bool late_interaction_update =
          Pixils::UI::dispatch_interactions(scoped_target->scope->view,
                                            scoped_target->scope->mouse_state,
                                            *scoped_target->focus_state,
                                            *hook_args.events,
                                            hook_args,
                                            roo_runtime,
                                            scoped_target->pass_depth);
        if (late_interaction_update)
        {
          Pixils::UI::update_view_tree(scoped_target->scope->view,
                                       scoped_target->scope->mouse_state,
                                       *scoped_target->focus_state,
                                       mouse_pos,
                                       hook_args,
                                       roo_runtime);
        }
        Pixils::UI::sync_focus_state(scoped_target->scope->view,
                                     *scoped_target->focus_state);
        Pixils::UI::refresh_view_interaction_tree(scoped_target->scope->view,
                                                  scoped_target->scope->mouse_state,
                                                  *scoped_target->focus_state,
                                                  mouse_pos);
        push_scope_state_to_root(scoped_target->scope->view, roo_runtime);
        mode_stack.update_state(scoped_target->owner_root->state,
                                scoped_target->mode_offset);
        mouse_state = scoped_target->scope->mouse_state;
      }
      else
      {
        auto interaction_root = active_mode;
        size_t interaction_offset = 0;
        if (active_mode && active_mode->mode &&
            active_mode->mode->composition.interaction_pass && !ctx_stack.empty())
        {
          interaction_root = ctx_stack.back();
          interaction_offset = 1;
        }

        bool late_interaction_update = Pixils::UI::dispatch_interactions(interaction_root,
                                                                         mouse_state,
                                                                         focus_state,
                                                                         *hook_args.events,
                                                                         hook_args,
                                                                         roo_runtime);
        if (late_interaction_update)
        {
          Pixils::UI::update_view_tree(interaction_root,
                                       mouse_state,
                                       focus_state,
                                       mouse_pos,
                                       hook_args,
                                       roo_runtime);
        }
        Pixils::UI::sync_focus_state(interaction_root, focus_state);
        Pixils::UI::refresh_view_interaction_tree(interaction_root,
                                                  mouse_state,
                                                  focus_state,
                                                  mouse_pos);
        if (interaction_offset > 0)
        {
          mode_stack.update_state(interaction_root->state, interaction_offset);
        }
      }
    }
    this->hook_args.update_state(active_mode->state);
  }

} // namespace Pixils::Runtime
