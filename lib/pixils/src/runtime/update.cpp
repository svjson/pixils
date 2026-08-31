#include <pixils/benchmark/counters.h>
#include <pixils/binding/ui/ui_namespace.h>
#include <pixils/hook_context.h>
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
      Session::ModeFrameMetadata::FrameComposition::InteractionScope* scope = nullptr;
      UI::FocusState* focus_state = nullptr;
      std::shared_ptr<View> owner_root;
      size_t mode_offset = 0;
    };

    struct InteractionScopeContext
    {
      HookContext& hook_context;
      std::shared_ptr<View> previous_scope;

      InteractionScopeContext(HookContext& hook_context,
                              const std::shared_ptr<View>& interaction_scope)
        : hook_context(hook_context)
        , previous_scope(hook_context.interaction_scope)
      {
        hook_context.interaction_scope = interaction_scope;
      }

      ~InteractionScopeContext() { hook_context.interaction_scope = previous_scope; }
    };

    enum class UpdateParticipation
    {
      TRANSPARENT,
      FULL,
      SCOPED,
    };

    struct UpdateFrame
    {
      UpdateParticipation participation = UpdateParticipation::TRANSPARENT;
      const std::vector<std::shared_ptr<View>>* scopes = nullptr;
      UI::FocusState* focus_state = nullptr;
      std::shared_ptr<View> owner_root;
      size_t mode_offset = 0;
      Session::ModeFrameMetadata::FrameComposition* composition = nullptr;
    };

    struct UpdateCompositionPlan
    {
      std::vector<UpdateFrame> frames;
    };

    UpdateCompositionPlan build_update_composition_plan(Session& session)
    {
      using UpdatePass = Session::ModeFrameMetadata::FrameComposition::UpdatePass;

      UpdateCompositionPlan plan;
      const size_t frame_count = session.mode_stack.size();
      if (frame_count <= 1 || session.frame_metadata.size() < frame_count ||
          session.ctx_stack.size() + 1 < frame_count)
      {
        return plan;
      }
      plan.frames.reserve(frame_count - 1);

      for (size_t child_frame = frame_count - 1; child_frame > 0; child_frame--)
      {
        auto& metadata = session.frame_metadata[child_frame];
        auto type = metadata.composition.update_pass.type;
        if (type == UpdatePass::Type::INHERIT)
        {
          auto child_root = child_frame == frame_count - 1 ? session.active_mode
                                                           : session.ctx_stack[child_frame];
          type = child_root && child_root->mode && child_root->mode->composition.update
                   ? UpdatePass::Type::FULL
                   : UpdatePass::Type::BLOCK;
        }
        if (type == UpdatePass::Type::BLOCK)
        {
          break;
        }

        const size_t owner_frame = child_frame - 1;
        const auto participation = type == UpdatePass::Type::FULL ? UpdateParticipation::FULL
                                   : metadata.composition.update_pass.scopes.empty()
                                     ? UpdateParticipation::TRANSPARENT
                                     : UpdateParticipation::SCOPED;
        plan.frames.push_back({.participation = participation,
                               .scopes = &metadata.composition.update_pass.scopes,
                               .focus_state = &metadata.restore_focus,
                               .owner_root = session.ctx_stack[owner_frame],
                               .mode_offset = frame_count - 1 - owner_frame,
                               .composition = &metadata.composition});
      }
      return plan;
    }

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
      for (size_t child_frame = frame_count - 1; child_frame > 0; child_frame--)
      {
        auto& metadata = session.frame_metadata[child_frame];
        auto& scopes = metadata.composition.interaction_pass;
        if (scopes.empty()) break;

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
            };
          }
        }
      }

      return std::nullopt;
    }

    void execute_update_plan(
      Session& session,
      const UpdateCompositionPlan& plan,
      const Point& mouse_pos,
      const std::optional<ScopedInteractionTarget>& interaction_target)
    {
      std::vector<CustomEvent> emitted_events;
      for (auto it = plan.frames.rbegin(); it != plan.frames.rend(); ++it)
      {
        if (it->participation != UpdateParticipation::FULL) continue;

        View& root = *it->owner_root;
        Roo::sptr_val_v args = session.hook_args.update_args;
        auto parent_state = root.state;
        emitted_events = UI::process_view_events(root,
                                                 &parent_state,
                                                 nullptr,
                                                 args.back(),
                                                 emitted_events,
                                                 session.roo_runtime);
        args[0] = root.state;
        if (root.definition->update &&
            root.definition->update->type != Roo::Value::Type::NIL)
        {
          root.set_state_if_changed(Runtime::invoke_hook(session.roo_runtime,
                                                         it->owner_root,
                                                         root.definition->update,
                                                         args,
                                                         root.state));
        }
        session.mode_stack.update_state(root.state, it->mode_offset);
        root.drain_events(emitted_events);
      }

      UI::MouseState empty_mouse_state;
      for (auto it = plan.frames.rbegin(); it != plan.frames.rend(); ++it)
      {
        if (it->participation != UpdateParticipation::SCOPED) continue;

        for (auto& scope : *it->scopes)
        {
          if (!belongs_to_root(scope, it->owner_root)) continue;
          if (interaction_target && interaction_target->scope->view == scope) continue;

          UI::MouseState* scope_mouse_state = nullptr;
          auto interaction_scope =
            std::find_if(it->composition->interaction_pass.begin(),
                         it->composition->interaction_pass.end(),
                         [&](const auto& candidate) { return candidate.view == scope; });
          if (interaction_scope != it->composition->interaction_pass.end())
          {
            scope_mouse_state = &interaction_scope->mouse_state;
          }

          UI::update_view_tree(scope,
                               scope_mouse_state ? *scope_mouse_state : empty_mouse_state,
                               *it->focus_state,
                               mouse_pos,
                               session.hook_args,
                               session.roo_runtime);
          push_scope_state_to_root(scope, session.roo_runtime);
        }
        session.mode_stack.update_state(it->owner_root->state, it->mode_offset);
      }
    }

    void relay_events(Session& session, const UpdateCompositionPlan& plan)
    {
      if (plan.frames.empty()) return;

      std::vector<CustomEvent> events;
      session.active_mode->drain_events(events);
      if (events.empty()) return;

      auto context = session.hook_args.update_args[1];
      for (auto& frame : plan.frames)
      {
        if (frame.participation == UpdateParticipation::FULL)
        {
          events = UI::process_view_events(*frame.owner_root,
                                           nullptr,
                                           nullptr,
                                           context,
                                           events,
                                           session.roo_runtime);
          frame.owner_root->drain_events(events);
        }
        else if (frame.participation == UpdateParticipation::SCOPED)
        {
          for (auto& scope : *frame.scopes)
          {
            if (!belongs_to_root(scope, frame.owner_root)) continue;
            events = UI::process_view_events(*scope,
                                             nullptr,
                                             nullptr,
                                             context,
                                             events,
                                             session.roo_runtime);
            scope->drain_events(events);
            push_scope_state_to_root(scope, session.roo_runtime);
            if (events.empty()) break;
          }
        }
        session.mode_stack.update_state(frame.owner_root->state, frame.mode_offset);
        if (events.empty()) break;
      }
    }

  } // namespace

  void Session::update_mode()
  {
    PIXILS_BENCHMARK_COUNT(runtime_update_mode_calls);
    PIXILS_BENCHMARK_TIME_BLOCK(runtime_update_time_ns);

    auto update_plan = build_update_composition_plan(*this);
    const Point mouse_pos = update_mouse_pos(hook_args);
    std::optional<ScopedInteractionTarget> scoped_target;

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
      scoped_target = has_pointer_transient(*hook_args.events)
                        ? scoped_interaction_target(*this, mouse_pos)
                        : std::nullopt;

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

        auto& native_hook_context = Roo::obj<HookContext>(*hook_args.update_args[1]);
        InteractionScopeContext interaction_scope(native_hook_context,
                                                  scoped_target->scope->view);
        bool late_interaction_update =
          Pixils::UI::dispatch_interactions(scoped_target->scope->view,
                                            scoped_target->scope->mouse_state,
                                            *scoped_target->focus_state,
                                            *hook_args.events,
                                            hook_args,
                                            roo_runtime);
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
    relay_events(*this, update_plan);
    execute_update_plan(*this, update_plan, mouse_pos, scoped_target);
    this->hook_args.update_state(active_mode->state);
  }

} // namespace Pixils::Runtime
