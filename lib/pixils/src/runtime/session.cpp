
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/view_events.h>
#include <pixils/ui/view_layout.h>
#include <pixils/ui/view_lifecycle.h>
#include <pixils/ui/view_render.h>

#include <SDL2/SDL_render.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    const Lisple::sptr_rtval KEYWORD__EVENT = Lisple::RTValue::keyword("event");
    const Lisple::sptr_rtval KEYWORD__ORIGIN = Lisple::RTValue::keyword("origin");
    const Lisple::sptr_rtval KEYWORD__POP_RESULT = Lisple::RTValue::keyword("pop/result");
    const Lisple::sptr_rtval KEYWORD__VIEW = Lisple::RTValue::keyword("view");

    Session::ModeFrameMetadata parse_frame_metadata(const Lisple::sptr_rtval& overrides)
    {
      Session::ModeFrameMetadata metadata;
      if (!overrides || overrides->type == Lisple::RTValue::Type::NIL)
      {
        return metadata;
      }

      auto origin = Lisple::Dict::get_property(overrides, KEYWORD__ORIGIN);
      if (!origin || origin->type == Lisple::RTValue::Type::NIL)
      {
        return metadata;
      }

      if (Script::HostType::VIEW.is_type_of(*origin))
      {
        metadata.origin_view = &Lisple::obj<View>(*origin);
        return metadata;
      }

      if (origin->type != Lisple::RTValue::Type::MAP)
      {
        return metadata;
      }

      auto view = Lisple::Dict::get_property(origin, KEYWORD__VIEW);
      if (view && Script::HostType::VIEW.is_type_of(*view))
      {
        metadata.origin_view = &Lisple::obj<View>(*view);
      }

      auto event = Lisple::Dict::get_property(origin, KEYWORD__EVENT);
      if (event && event->type != Lisple::RTValue::Type::NIL)
      {
        metadata.origin_event = event;
      }

      return metadata;
    }

    bool find_view_path(const std::shared_ptr<View>& current,
                        View* target,
                        std::vector<std::shared_ptr<View>>& path)
    {
      if (!current)
      {
        return false;
      }

      if (current.get() == target)
      {
        path.push_back(current);
        return true;
      }

      for (auto& child : current->children)
      {
        if (find_view_path(child, target, path))
        {
          path.push_back(current);
          return true;
        }
      }

      return false;
    }

    void dispatch_event_along_path(const std::vector<std::shared_ptr<View>>& path,
                                   const CustomEvent& event,
                                   const Lisple::sptr_rtval& view_ctx,
                                   Lisple::Runtime& runtime)
    {
      std::vector<CustomEvent> events = {event};
      auto hook_ctx = view_ctx;

      for (size_t i = 0; i < path.size() && !events.empty(); i++)
      {
        Lisple::sptr_rtval* parent_state =
          i + 1 < path.size() ? &path[i + 1]->state : nullptr;
        events = UI::process_view_events(*path[i], parent_state, hook_ctx, events, runtime);
      }
    }

  } // namespace

  Session::Session(Lisple::Runtime& lisple_runtime,
                   Asset::Registry& assets,
                   RenderContext& render_ctx,
                   const HookArguments& hook_args)
    : lisple_runtime(lisple_runtime)
    , assets(assets)
    , render_ctx(render_ctx)
    , mode_stack(lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK),
                 lisple_runtime.lookup_value(Script::ID__PIXILS__MODE_STACK_MESSAGES))
    , modes(lisple_runtime.lookup_value(Script::ID__PIXILS__MODES))
    , hook_args(hook_args)
  {
  }

  void Session::pop_mode(const Lisple::sptr_rtval& payload)
  {
    if (mode_stack.size() > 1)
    {
      auto popped_frame = mode_stack.peek();
      auto* popped_mode = popped_frame.first;
      auto frame_meta = frame_metadata.empty() ? ModeFrameMetadata{} : frame_metadata.back();

      mode_stack.pop();
      if (!frame_metadata.empty())
      {
        frame_metadata.pop_back();
      }

      /**
       * Restore active_mode from the context stack. Then re-sync
       * state from the Lisple stack (which holds the authoritative
       * saved state) and restore each child's state slice from that
       * parent state.
       */
      active_mode = ctx_stack.back();
      ctx_stack.pop_back();

      auto restored_frame = mode_stack.peek();
      auto saved_state = restored_frame.second;
      active_mode->state = saved_state;
      for (auto& child : active_mode->children)
      {
        Pixils::UI::restore_view_tree(child, active_mode->state);
      }

      auto pop_event_key = frame_meta.origin_event->type != Lisple::RTValue::Type::NIL
                             ? frame_meta.origin_event
                             : KEYWORD__POP_RESULT;
      auto pop_event_source_mode = Lisple::RTValue::symbol(popped_mode->name);
      std::vector<std::shared_ptr<View>> path;

      if (frame_meta.origin_view &&
          find_view_path(active_mode, frame_meta.origin_view, path))
      {
        dispatch_event_along_path(path,
                                  CustomEvent(pop_event_key,
                                              payload ? payload : Lisple::Constant::NIL,
                                              pop_event_source_mode),
                                  hook_args.update_args[1],
                                  lisple_runtime);
      }
      else if (active_mode)
      {
        dispatch_event_along_path({active_mode},
                                  CustomEvent(pop_event_key,
                                              payload ? payload : Lisple::Constant::NIL,
                                              pop_event_source_mode),
                                  hook_args.update_args[1],
                                  lisple_runtime);
      }

      mode_stack.update_state(active_mode->state);
      this->hook_args.update_state(active_mode->state);
    }
  }

  void Session::push_mode(const Lisple::sptr_rtval& mode,
                          const Lisple::sptr_rtval& state,
                          const Lisple::sptr_rtval& overrides)
  {
    std::optional<UI::Theme> inherited_theme = std::nullopt;
    /**
     * Flush the current active context's state to the Lisple stack before pushing,
     * then save the context itself so pop_mode can recover it cheaply.
     */
    if (mode_stack.size() > 0)
    {
      inherited_theme = active_mode->effective_theme;
      mode_stack.update_state(active_mode->state);
      ctx_stack.push_back(std::move(active_mode));
    }
    frame_metadata.push_back(parse_frame_metadata(overrides));
    this->mode_stack.push(mode, state);

    auto& mode_obj = Lisple::obj<Mode>(*mode);

    active_mode = Pixils::UI::build_root_view(mode_obj, state, overrides, lisple_runtime);
    active_mode->inherited_theme = inherited_theme;

    this->hook_args.update_state(state);

    Pixils::UI::init_root_view(assets, lisple_runtime, hook_args.init_args[1], active_mode);
    this->hook_args.update_state(active_mode->state);

    /**
     * Build children and initialize each child, threading child states into
     * the parent state map as they complete.
     */
    auto parent_state = this->active_mode->state;
    for (const auto& slot : active_mode->mode->children)
    {
      this->active_mode->children.push_back(
        Pixils::UI::build_view_tree(slot, modes, lisple_runtime));
      parent_state = Pixils::UI::init_view_tree(assets,
                                                lisple_runtime,
                                                hook_args.init_args[1],
                                                this->active_mode->children.back(),
                                                parent_state);
    }

    this->active_mode->state = parent_state;
    this->hook_args.update_state(parent_state);
  }

  void Session::push_mode(const std::string& mode_name,
                          const Lisple::sptr_rtval& state,
                          const Lisple::sptr_rtval& overrides)
  {
    auto mode = Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(mode_name));
    this->push_mode(mode, state, overrides);
  }

  void Session::process_messages()
  {
    Lisple::sptr_rtval_v messages = mode_stack.drain_messages();

    for (auto& message : messages)
    {
      std::string type =
        Lisple::Dict::get_property(message, Lisple::RTValue::keyword("type"))->str();

      if (type == "push")
      {
        mode_stack.update_state(active_mode->state);
        auto overrides_val =
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("overrides"));
        push_mode(
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("mode"))->str(),
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("state")),
          overrides_val ? overrides_val : Lisple::Constant::NIL);
      }
      else if (type == "pop")
      {
        auto payload =
          Lisple::Dict::get_property(message, Lisple::RTValue::keyword("payload"));
        pop_mode(payload ? payload : Lisple::Constant::NIL);
      }
      else if (type == "quit")
      {
        quit_requested = true;
      }
    }
  }

  void Session::render_mode()
  {
    auto render_stack = mode_stack.get_render_stack();
    Rect full = {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h};

    /**
     * render_stack is top-first: [0]=top, [1]=just below, ..., [n-1]=bottom.
     * Render from bottom up, skipping index 0 (active_mode, rendered last).
     */
    for (size_t i = render_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      Pixils::UI::layout_view_tree(ctx_stack[ctx_idx],
                                   full,
                                   lisple_runtime,
                                   hook_args.render_args[1]);
      Pixils::UI::render_view(render_ctx,
                              lisple_runtime,
                              hook_args.render_args[1],
                              ctx_stack[ctx_idx]);
    }

    Pixils::UI::layout_view_tree(active_mode,
                                 full,
                                 lisple_runtime,
                                 hook_args.render_args[1]);
    Pixils::UI::render_view(render_ctx,
                            lisple_runtime,
                            hook_args.render_args[1],
                            active_mode);
  }
} // namespace Pixils::Runtime
