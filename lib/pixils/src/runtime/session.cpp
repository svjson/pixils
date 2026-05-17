
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/interaction_dispatch.h>
#include <pixils/ui/view_events.h>
#include <pixils/ui/view_layout.h>
#include <pixils/ui/view_lifecycle.h>
#include <pixils/ui/view_render.h>
#include <pixils/ui/view_update.h>

#include <SDL2/SDL_render.h>
#include <lisple/exception.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::Runtime
{
  namespace
  {
    const Lisple::sptr_val KEYWORD__EVENT = Lisple::keyword("event");
    const Lisple::sptr_val KEYWORD__ORIGIN = Lisple::keyword("origin");
    const Lisple::sptr_val KEYWORD__POP_RESULT = Lisple::keyword("pop/result");
    const Lisple::sptr_val KEYWORD__TARGET = Lisple::keyword("target");
    const Lisple::sptr_val KEYWORD__VIEW = Lisple::keyword("view");

    Session::ModeFrameMetadata parse_frame_metadata(const Lisple::sptr_val& overrides)
    {
      Session::ModeFrameMetadata metadata;
      if (!overrides || overrides->type == Lisple::Value::Type::NIL)
      {
        return metadata;
      }

      auto origin = Lisple::Dict::get_property(overrides, KEYWORD__ORIGIN);
      if (!origin || origin->type == Lisple::Value::Type::NIL)
      {
        return metadata;
      }

      if (Script::HostType::VIEW.is_type_of(*origin))
      {
        metadata.origin_view = &Lisple::obj<View>(*origin);
        return metadata;
      }

      if (origin->type != Lisple::Value::Type::MAP)
      {
        return metadata;
      }

      auto view = Lisple::Dict::get_property(origin, KEYWORD__VIEW);
      if (view && Script::HostType::VIEW.is_type_of(*view))
      {
        metadata.origin_view = &Lisple::obj<View>(*view);
      }

      auto event = Lisple::Dict::get_property(origin, KEYWORD__EVENT);
      if (event && event->type != Lisple::Value::Type::NIL)
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

    void store_focus_chain(UI::FocusState& focus_state,
                           const std::vector<std::shared_ptr<View>>& chain)
    {
      focus_state.clear();
      if (chain.empty()) return;

      focus_state.focused = chain[0];
      for (auto& view : chain)
      {
        focus_state.focus_chain.push_back(std::weak_ptr<View>(view));
      }
    }

    View* resolve_target_view(const Lisple::sptr_val& value)
    {
      if (!value || value->type == Lisple::Value::Type::NIL)
      {
        return nullptr;
      }

      if (!Script::HostType::VIEW.is_type_of(*value))
      {
        return nullptr;
      }

      return &Lisple::obj<View>(*value);
    }

    bool focus_target_view(const std::shared_ptr<View>& root,
                           UI::FocusState& focus_state,
                           View* target)
    {
      if (!root || !target)
      {
        return false;
      }

      std::vector<std::shared_ptr<View>> path;
      if (!find_view_path(root, target, path))
      {
        return false;
      }

      if (!path.front()->mode || !path.front()->mode->focusable)
      {
        return false;
      }

      store_focus_chain(focus_state, path);
      return true;
    }

    Point current_mouse_pos(const HookArguments& hook_args)
    {
      if (!hook_args.events || !hook_args.events->mouse_pos ||
          hook_args.events->mouse_pos->type == Lisple::Value::Type::NIL)
      {
        return {0.0f, 0.0f};
      }

      return Lisple::obj<Point>(*hook_args.events->mouse_pos);
    }

    void dispatch_event_along_path(const std::vector<std::shared_ptr<View>>& path,
                                   const CustomEvent& event,
                                   const Lisple::sptr_val& view_ctx,
                                   Lisple::Runtime& runtime)
    {
      std::vector<CustomEvent> events = {event};
      auto hook_ctx = view_ctx;

      for (size_t i = 0; i < path.size() && !events.empty(); i++)
      {
        Lisple::sptr_val* parent_state = i + 1 < path.size() ? &path[i + 1]->state : nullptr;
        events = UI::process_view_events(*path[i], parent_state, hook_ctx, events, runtime);
      }
    }

    Lisple::sptr_val resolve_mode_value(const Lisple::sptr_val& modes,
                                        const std::string& mode_name)
    {
      auto mode = Lisple::Dict::get_property(modes, Lisple::symbol(mode_name));
      if (!mode || mode->type == Lisple::Value::Type::NIL)
      {
        throw Lisple::InvocationException("Unknown mode '" + mode_name + "'");
      }

      if (!Script::HostType::MODE.is_type_of(*mode))
      {
        throw Lisple::InvocationException("Identifier '" + mode_name +
                                          "' resolved to non-mode value");
      }

      return mode;
    }

  } // namespace

  Session::Session(Lisple::Runtime& lisple_runtime,
                   Asset::Registry& assets,
                   RenderContext& render_ctx,
                   const HookArguments& hook_args)
    : lisple_runtime(lisple_runtime)
    , assets(assets)
    , render_ctx(render_ctx)
    , mode_stack(lisple_runtime.lookup(Script::ID__PIXILS__MODE_STACK),
                 lisple_runtime.lookup(Script::ID__PIXILS__MODE_STACK_MESSAGES))
    , modes(lisple_runtime.lookup(Script::ID__PIXILS__MODES))
    , hook_args(hook_args)
  {
  }

  void Session::pop_mode(const Lisple::sptr_val& payload)
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
      focus_state = frame_meta.restore_focus;

      auto pop_event_key = frame_meta.origin_event->type != Lisple::Value::Type::NIL
                             ? frame_meta.origin_event
                             : KEYWORD__POP_RESULT;
      auto pop_event_source_mode = Lisple::symbol(popped_mode->name);
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
      UI::sync_focus_state(active_mode, focus_state);
      UI::refresh_view_interaction_tree(active_mode,
                                        mouse_state,
                                        focus_state,
                                        current_mouse_pos(hook_args));
    }
  }

  void Session::push_mode(const Lisple::sptr_val& mode,
                          const Lisple::sptr_val& state,
                          const Lisple::sptr_val& overrides)
  {
    std::optional<UI::Theme> inherited_theme = std::nullopt;
    /**
     * Flush the current active context's state to the Lisple stack before pushing,
     * then save the context itself so pop_mode can recover it cheaply.
     */
    if (mode_stack.size() > 0)
    {
      inherited_theme =
        UI::resolve_effective_theme(active_mode,
                                    lisple_runtime,
                                    active_mode->inherited_theme
                                      ? &*active_mode->inherited_theme
                                      : nullptr);
      mode_stack.update_state(active_mode->state);
      ctx_stack.push_back(std::move(active_mode));
    }
    auto metadata = parse_frame_metadata(overrides);
    metadata.restore_focus = focus_state;
    frame_metadata.push_back(std::move(metadata));
    focus_state.clear();
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
                          const Lisple::sptr_val& state,
                          const Lisple::sptr_val& overrides)
  {
    auto mode = resolve_mode_value(modes, mode_name);
    this->push_mode(mode, state, overrides);
  }

  void Session::process_messages()
  {
    bool focus_changed = false;

    while (true)
    {
      Lisple::sptr_val_v messages = mode_stack.drain_messages();
      if (messages.empty())
      {
        break;
      }

      for (auto& message : messages)
      {
        std::string type =
          Lisple::Dict::get_property(message, Lisple::keyword("type"))->str();

        if (type == "push")
        {
          mode_stack.update_state(active_mode->state);
          auto overrides_val =
            Lisple::Dict::get_property(message, Lisple::keyword("overrides"));
          push_mode(Lisple::Dict::get_property(message, Lisple::keyword("mode"))->str(),
                    Lisple::Dict::get_property(message, Lisple::keyword("state")),
                    overrides_val ? overrides_val : Lisple::Constant::NIL);
        }
        else if (type == "pop")
        {
          auto payload = Lisple::Dict::get_property(message, Lisple::keyword("payload"));
          pop_mode(payload ? payload : Lisple::Constant::NIL);
        }
        else if (type == "focus")
        {
          auto target =
            resolve_target_view(Lisple::Dict::get_property(message, KEYWORD__TARGET));
          if (focus_target_view(active_mode, focus_state, target))
          {
            focus_changed = true;
          }
        }
        else if (type == "blur")
        {
          auto target =
            resolve_target_view(Lisple::Dict::get_property(message, KEYWORD__TARGET));
          if (!target)
          {
            focus_state.clear();
            focus_changed = true;
          }
          else if (auto focused = focus_state.focused.lock();
                   focused && focused.get() == target)
          {
            focus_state.clear();
            focus_changed = true;
          }
        }
        else if (type == "quit")
        {
          quit_requested = true;
        }
      }
    }

    if (focus_changed)
    {
      UI::refresh_view_interaction_tree(active_mode,
                                        mouse_state,
                                        focus_state,
                                        current_mouse_pos(hook_args));
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
