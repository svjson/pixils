
#include "pixils/runtime/session.h"

#include <pixils/asset/registry.h>
#include <pixils/benchmark/counters.h>
#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/point_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/state.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/base_theme.h>
#include <pixils/ui/interaction_dispatch.h>
#include <pixils/ui/view_events.h>
#include <pixils/ui/view_layout.h>
#include <pixils/ui/view_lifecycle.h>
#include <pixils/ui/view_render.h>
#include <pixils/ui/view_update.h>

#include <SDL3/SDL_render.h>
#include <roo/exception.h>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

#include <algorithm>

namespace Pixils::Runtime
{
  namespace
  {
    const Roo::sptr_val KEYWORD__ANCHOR = Roo::keyword("anchor");
    const Roo::sptr_val KEYWORD__ANCHOR_BOUNDS = Roo::keyword("anchor-bounds");
    const Roo::sptr_val KEYWORD__EVENT = Roo::keyword("event");
    const Roo::sptr_val KEYWORD__FALLBACK_PLACEMENT = Roo::keyword("fallback-placement");
    const Roo::sptr_val KEYWORD__MATCH_ANCHOR_WIDTH =
      Roo::keyword("match-anchor-width?");
    const Roo::sptr_val KEYWORD__OVERLAY = Roo::keyword("overlay");
    const Roo::sptr_val KEYWORD__ORIGIN = Roo::keyword("origin");
    const Roo::sptr_val KEYWORD__PLACEMENT = Roo::keyword("placement");
    const Roo::sptr_val KEYWORD__POP_RESULT = Roo::keyword("pop/result");
    const Roo::sptr_val KEYWORD__TARGET = Roo::keyword("target");
    const Roo::sptr_val KEYWORD__THEME = Roo::keyword("theme");
    const Roo::sptr_val KEYWORD__THEME_VARIANT = Roo::keyword("theme-variant");
    const Roo::sptr_val KEYWORD__VIEWPORT_PADDING = Roo::keyword("viewport-padding");
    const Roo::sptr_val KEYWORD__VIEW = Roo::keyword("view");

    Session::ModeFrameMetadata::OverlayPlacement parse_overlay_placement(
      const Roo::sptr_val& value,
      Session::ModeFrameMetadata::OverlayPlacement fallback)
    {
      if (!value || value->type == Roo::Value::Type::NIL)
      {
        return fallback;
      }

      if (value->type != Roo::Value::Type::KEYWORD &&
          value->type != Roo::Value::Type::SYMBOL)
      {
        throw Roo::TypeError("Overlay placement must be a keyword");
      }

      const auto name = value->str();
      if (name == "bottom-start")
      {
        return Session::ModeFrameMetadata::OverlayPlacement::BOTTOM_START;
      }
      if (name == "top-start")
      {
        return Session::ModeFrameMetadata::OverlayPlacement::TOP_START;
      }
      if (name == "none")
      {
        return Session::ModeFrameMetadata::OverlayPlacement::NONE;
      }

      throw Roo::InvocationException("Unknown overlay placement '" + name + "'");
    }

    bool parse_bool_option(const Roo::sptr_val& value, bool fallback, const char* label)
    {
      if (!value || value->type == Roo::Value::Type::NIL)
      {
        return fallback;
      }
      if (value->type != Roo::Value::Type::BOOL)
      {
        throw Roo::TypeError(std::string(label) + " must be a boolean");
      }
      return std::get<bool>(value->value);
    }

    int parse_int_option(const Roo::sptr_val& value, int fallback, const char* label)
    {
      if (!value || value->type == Roo::Value::Type::NIL)
      {
        return fallback;
      }
      if (value->type != Roo::Value::Type::NUMBER)
      {
        throw Roo::TypeError(std::string(label) + " must be a number");
      }
      return std::max(0, value->num().get_int());
    }

    std::optional<Session::ModeFrameMetadata::Overlay> parse_overlay_metadata(
      const Roo::sptr_val& overrides)
    {
      auto overlay = Roo::Dict::get_property(overrides, KEYWORD__OVERLAY);
      if (!overlay || overlay->type == Roo::Value::Type::NIL)
      {
        return std::nullopt;
      }
      if (overlay->type != Roo::Value::Type::MAP)
      {
        throw Roo::TypeError("Mode :overlay metadata must be a map");
      }

      auto anchor = Roo::Dict::get_property(overlay, KEYWORD__ANCHOR);
      auto anchor_bounds = Roo::Dict::get_property(overlay, KEYWORD__ANCHOR_BOUNDS);
      if ((!anchor || anchor->type == Roo::Value::Type::NIL) &&
          (!anchor_bounds || anchor_bounds->type == Roo::Value::Type::NIL))
      {
        throw Roo::TypeError("Mode :overlay requires :anchor or :anchor-bounds");
      }
      if (anchor && anchor->type != Roo::Value::Type::NIL &&
          !Script::HostType::VIEW.is_type_of(*anchor))
      {
        throw Roo::TypeError("Mode :overlay :anchor must be a view");
      }
      if (anchor_bounds && anchor_bounds->type != Roo::Value::Type::NIL &&
          !Script::HostType::RECT.is_type_of(*anchor_bounds))
      {
        throw Roo::TypeError("Mode :overlay :anchor-bounds must be a rect");
      }

      Session::ModeFrameMetadata::Overlay metadata;
      if (anchor && anchor->type != Roo::Value::Type::NIL)
      {
        metadata.anchor_view = &Roo::obj<View>(*anchor);
      }
      if (anchor_bounds && anchor_bounds->type != Roo::Value::Type::NIL)
      {
        metadata.anchor_bounds = Roo::obj<Rect>(*anchor_bounds);
      }
      metadata.placement = parse_overlay_placement(
        Roo::Dict::get_property(overlay, KEYWORD__PLACEMENT),
        Session::ModeFrameMetadata::OverlayPlacement::BOTTOM_START);
      metadata.fallback_placement = parse_overlay_placement(
        Roo::Dict::get_property(overlay, KEYWORD__FALLBACK_PLACEMENT),
        Session::ModeFrameMetadata::OverlayPlacement::NONE);
      metadata.match_anchor_width = parse_bool_option(
        Roo::Dict::get_property(overlay, KEYWORD__MATCH_ANCHOR_WIDTH),
        false,
        "Mode :overlay :match-anchor-width?");
      metadata.viewport_padding = parse_int_option(
        Roo::Dict::get_property(overlay, KEYWORD__VIEWPORT_PADDING),
        0,
        "Mode :overlay :viewport-padding");
      return metadata;
    }

    Session::ModeFrameMetadata parse_frame_metadata(const Roo::sptr_val& overrides)
    {
      Session::ModeFrameMetadata metadata;
      if (!overrides || overrides->type == Roo::Value::Type::NIL)
      {
        return metadata;
      }

      metadata.overlay = parse_overlay_metadata(overrides);

      auto origin = Roo::Dict::get_property(overrides, KEYWORD__ORIGIN);
      if (!origin || origin->type == Roo::Value::Type::NIL)
      {
        return metadata;
      }

      if (Script::HostType::VIEW.is_type_of(*origin))
      {
        metadata.origin_view = &Roo::obj<View>(*origin);
        return metadata;
      }

      if (origin->type != Roo::Value::Type::MAP)
      {
        return metadata;
      }

      auto view = Roo::Dict::get_property(origin, KEYWORD__VIEW);
      if (view && Script::HostType::VIEW.is_type_of(*view))
      {
        metadata.origin_view = &Roo::obj<View>(*view);
      }

      auto event = Roo::Dict::get_property(origin, KEYWORD__EVENT);
      if (event && event->type != Roo::Value::Type::NIL)
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

    View* resolve_target_view(const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL)
      {
        return nullptr;
      }

      if (!Script::HostType::VIEW.is_type_of(*value))
      {
        return nullptr;
      }

      return &Roo::obj<View>(*value);
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
          hook_args.events->mouse_pos->type == Roo::Value::Type::NIL)
      {
        return {0.0f, 0.0f};
      }

      return Roo::obj<Point>(*hook_args.events->mouse_pos);
    }

    void dispatch_event_along_path(const std::vector<std::shared_ptr<View>>& path,
                                   const CustomEvent& event,
                                   const Roo::sptr_val& view_ctx,
                                   Roo::Runtime& runtime)
    {
      std::vector<CustomEvent> events = {event};
      auto hook_ctx = view_ctx;

      for (size_t i = 0; i < path.size() && !events.empty(); i++)
      {
        Roo::sptr_val* parent_state = i + 1 < path.size() ? &path[i + 1]->state : nullptr;
        Runtime::View* parent_view = i + 1 < path.size() ? path[i + 1].get() : nullptr;
        events = UI::process_view_events(*path[i],
                                         parent_state,
                                         parent_view,
                                         hook_ctx,
                                         events,
                                         runtime);
      }
    }

    Roo::sptr_val resolve_mode_value(const Roo::sptr_val& modes,
                                        const std::string& mode_name)
    {
      auto mode = Roo::Dict::get_property(modes, Roo::symbol(mode_name));
      if (!mode || mode->type == Roo::Value::Type::NIL)
      {
        throw Roo::InvocationException("Unknown mode '" + mode_name + "'");
      }

      if (!Script::HostType::MODE.is_type_of(*mode))
      {
        throw Roo::InvocationException("Identifier '" + mode_name +
                                          "' resolved to non-mode value");
      }

      return mode;
    }

    std::optional<UI::Theme> lookup_theme(Roo::Runtime& runtime, const std::string& name)
    {
      auto themes = runtime.lookup(Script::ID__PIXILS__THEMES);
      auto theme_val = Roo::Dict::get_property(themes, Roo::symbol(name));
      if (!theme_val || theme_val->type == Roo::Value::Type::NIL) return std::nullopt;
      return Roo::obj<UI::Theme>(*theme_val);
    }

    void invalidate_style_tree(const std::shared_ptr<View>& view)
    {
      if (!view) return;

      view->style_view.invalidate_theme();
      view->mark_style_changed();
      for (auto& child : view->children)
      {
        invalidate_style_tree(child);
      }
    }

    std::shared_ptr<View> overlay_target_for_root(const std::shared_ptr<View>& root)
    {
      if (!root) return nullptr;
      if (!root->children.empty()) return root->children.front();
      return root;
    }

    int overlay_top_for_placement(Session::ModeFrameMetadata::OverlayPlacement placement,
                                  const Rect& anchor,
                                  int target_visual_height)
    {
      switch (placement)
      {
      case Session::ModeFrameMetadata::OverlayPlacement::TOP_START:
        return anchor.y - target_visual_height;
      case Session::ModeFrameMetadata::OverlayPlacement::BOTTOM_START:
        return anchor.y + anchor.h;
      case Session::ModeFrameMetadata::OverlayPlacement::NONE:
        break;
      }
      return anchor.y + anchor.h;
    }

    bool placement_fits_vertically(int visual_top,
                                   int visual_height,
                                   const Rect& viewport,
                                   int padding)
    {
      return visual_top >= viewport.y + padding &&
             visual_top + visual_height <= viewport.y + viewport.h - padding;
    }

    void offset_visual_subtree(const std::shared_ptr<View>& view,
                               int dx,
                               int dy,
                               int visual_dx,
                               int visual_dy)
    {
      if (!view) return;
      view->bounds.x += dx;
      view->bounds.y += dy;
      view->external_bounds.x += dx;
      view->external_bounds.y += dy;
      view->visual_bounds.x += visual_dx;
      view->visual_bounds.y += visual_dy;
      for (auto& child : view->children)
      {
        offset_visual_subtree(child, dx, dy, visual_dx, visual_dy);
      }
    }

    bool apply_overlay_placement(const std::shared_ptr<View>& root,
                                 const Session::ModeFrameMetadata& metadata,
                                 const Rect& viewport)
    {
      if (!metadata.overlay ||
          (!metadata.overlay->anchor_view && !metadata.overlay->anchor_bounds))
      {
        return false;
      }

      auto target = overlay_target_for_root(root);
      if (!target)
      {
        return false;
      }

      const auto& overlay = *metadata.overlay;
      const Rect anchor =
        overlay.anchor_bounds ? *overlay.anchor_bounds : overlay.anchor_view->visual_bounds;
      const int scale = std::max(1, root ? root->visual_scale : target->visual_scale);
      const int target_visual_width =
        overlay.match_anchor_width ? anchor.w : target->visual_bounds.w;
      const int target_visual_height = target->visual_bounds.h;
      int visual_left = anchor.x;
      int visual_top = overlay_top_for_placement(overlay.placement,
                                                 anchor,
                                                 target_visual_height);

      if (overlay.fallback_placement !=
            Session::ModeFrameMetadata::OverlayPlacement::NONE &&
          !placement_fits_vertically(visual_top,
                                     target_visual_height,
                                     viewport,
                                     overlay.viewport_padding))
      {
        int fallback_top = overlay_top_for_placement(overlay.fallback_placement,
                                                     anchor,
                                                     target_visual_height);
        if (placement_fits_vertically(fallback_top,
                                      target_visual_height,
                                      viewport,
                                      overlay.viewport_padding))
        {
          visual_top = fallback_top;
        }
      }

      const int max_left =
        viewport.x + viewport.w - overlay.viewport_padding - target_visual_width;
      visual_left = std::clamp(visual_left,
                               viewport.x + overlay.viewport_padding,
                               std::max(viewport.x + overlay.viewport_padding, max_left));

      const int max_top =
        viewport.y + viewport.h - overlay.viewport_padding - target_visual_height;
      visual_top = std::clamp(visual_top,
                              viewport.y + overlay.viewport_padding,
                              std::max(viewport.y + overlay.viewport_padding, max_top));

      const int logical_left = visual_left / scale;
      const int logical_top = visual_top / scale;
      const int logical_width = std::max(1, target_visual_width / scale);

      int dx = logical_left - target->bounds.x;
      int dy = logical_top - target->bounds.y;
      bool changed = dx != 0 || dy != 0;
      if (overlay.match_anchor_width && target->bounds.w != logical_width)
      {
        target->bounds.w = logical_width;
        target->external_bounds.w = logical_width;
        target->visual_bounds.w = target_visual_width;
        changed = true;
      }
      if (dx != 0 || dy != 0)
      {
        offset_visual_subtree(target,
                              dx,
                              dy,
                              visual_left - target->visual_bounds.x,
                              visual_top - target->visual_bounds.y);
      }
      return changed;
    }

  } // namespace

  Session::Session(Roo::Runtime& roo_runtime,
                   Asset::Registry& assets,
                   RenderContext& render_ctx,
                   const HookArguments& hook_args)
    : roo_runtime(roo_runtime)
    , assets(assets)
    , render_ctx(render_ctx)
    , mode_stack(roo_runtime.lookup(Script::ID__PIXILS__MODE_STACK),
                 roo_runtime.lookup(Script::ID__PIXILS__MODE_STACK_MESSAGES))
    , modes(roo_runtime.lookup(Script::ID__PIXILS__MODES))
    , hook_args(hook_args)
  {
  }

  void Session::pop_mode(const Roo::sptr_val& payload)
  {
    PIXILS_BENCHMARK_COUNT(runtime_pop_mode_calls);

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
       * state from the Roo stack (which holds the authoritative
       * saved state) and restore each child's state slice from that
       * parent state.
       */
      active_mode = ctx_stack.back();
      ctx_stack.pop_back();

      auto restored_frame = mode_stack.peek();
      auto saved_state = restored_frame.second;
      active_mode->set_state_if_changed(saved_state);
      for (auto& child : active_mode->children)
      {
        Pixils::UI::restore_view_tree(child, active_mode->state);
      }
      focus_state = frame_meta.restore_focus;

      auto pop_event_key = frame_meta.origin_event->type != Roo::Value::Type::NIL
                             ? frame_meta.origin_event
                             : KEYWORD__POP_RESULT;
      auto pop_event_source_mode = Roo::symbol(popped_mode->name);
      std::vector<std::shared_ptr<View>> path;

      if (frame_meta.origin_view &&
          find_view_path(active_mode, frame_meta.origin_view, path))
      {
        dispatch_event_along_path(path,
                                  CustomEvent(pop_event_key,
                                              payload ? payload : Roo::Constant::NIL,
                                              pop_event_source_mode),
                                  hook_args.update_args[1],
                                  roo_runtime);
      }
      else if (active_mode)
      {
        dispatch_event_along_path({active_mode},
                                  CustomEvent(pop_event_key,
                                              payload ? payload : Roo::Constant::NIL,
                                              pop_event_source_mode),
                                  hook_args.update_args[1],
                                  roo_runtime);
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

  void Session::push_mode(const Roo::sptr_val& mode,
                          const Roo::sptr_val& state,
                          const Roo::sptr_val& overrides)
  {
    PIXILS_BENCHMARK_COUNT(runtime_push_mode_calls);

    std::optional<UI::Theme> inherited_theme = resolved_application_theme;
    /**
     * Flush the current active context's state to the Roo stack before pushing,
     * then save the context itself so pop_mode can recover it cheaply.
     */
    if (mode_stack.size() > 0)
    {
      inherited_theme = UI::resolve_effective_theme(
        active_mode,
        roo_runtime,
        active_mode->inherited_theme ? &*active_mode->inherited_theme : nullptr);
      mode_stack.update_state(active_mode->state);
      ctx_stack.push_back(std::move(active_mode));
    }
    auto metadata = parse_frame_metadata(overrides);
    metadata.restore_focus = focus_state;
    frame_metadata.push_back(std::move(metadata));
    focus_state.clear();
    this->mode_stack.push(mode, state);

    auto& mode_obj = Roo::obj<Mode>(*mode);

    active_mode = Pixils::UI::build_root_view(mode_obj, state, overrides, roo_runtime);
    active_mode->inherited_theme = inherited_theme;

    this->hook_args.update_state(state);

    Pixils::UI::init_root_view(assets, roo_runtime, hook_args.init_args[1], active_mode);
    this->hook_args.update_state(active_mode->state);

    /**
     * Build children and initialize each child, threading child states into
     * the parent state map as they complete.
     */
    auto parent_state = this->active_mode->state;
    for (const auto& slot : active_mode->mode->children)
    {
      this->active_mode->children.push_back(
        Pixils::UI::build_view_tree(slot, modes, roo_runtime));
      this->active_mode->mark_children_changed();
      Pixils::UI::attach_style_view_tree(this->active_mode->children.back(),
                                         this->active_mode.get());
      parent_state = Pixils::UI::init_view_tree(assets,
                                                roo_runtime,
                                                hook_args.init_args[1],
                                                this->active_mode->children.back(),
                                                parent_state);
    }

    this->active_mode->set_state_if_changed(parent_state);
    this->hook_args.update_state(parent_state);
  }

  void Session::push_mode(const std::string& mode_name,
                          const Roo::sptr_val& state,
                          const Roo::sptr_val& overrides)
  {
    auto mode = resolve_mode_value(modes, mode_name);
    this->push_mode(mode, state, overrides);
  }

  void Session::set_application_theme(const std::optional<std::vector<std::string>>& theme,
                                      const std::optional<std::string>& variant)
  {
    application_theme = theme;
    application_theme_variant = variant;
    resolved_application_theme = std::nullopt;

    if (application_theme || application_theme_variant)
    {
      UI::Theme resolved = UI::default_base_theme(roo_runtime)
                             .resolved_for_variant(application_theme_variant);
      if (application_theme)
      {
        for (const auto& theme_name : *application_theme)
        {
          auto local_theme = lookup_theme(roo_runtime, theme_name);
          if (local_theme)
          {
            UI::overlay_theme(resolved,
                              local_theme->resolved_for_variant(application_theme_variant));
          }
        }
      }

      resolved.selected_variant = application_theme_variant;
      Roo::Context ctx(roo_runtime);
      resolved_application_theme =
        Pixils::Script::resolve_theme_declarations(ctx, resolved, application_theme_variant);
    }

    std::optional<UI::Theme> inherited_theme = resolved_application_theme;
    auto apply_to_root = [&](const std::shared_ptr<View>& root)
    {
      if (!root) return;

      root->inherited_theme = inherited_theme;
      invalidate_style_tree(root);
      inherited_theme = UI::resolve_effective_theme(
        root,
        roo_runtime,
        root->inherited_theme ? &*root->inherited_theme : nullptr);
    };

    for (auto& root : ctx_stack)
    {
      apply_to_root(root);
    }
    apply_to_root(active_mode);
  }

  bool Session::process_messages()
  {
    bool active_frame_changed = false;
    bool focus_changed = false;

    while (true)
    {
      Roo::sptr_val_v messages = mode_stack.drain_messages();
      if (messages.empty())
      {
        break;
      }

      for (auto& message : messages)
      {
        std::string type =
          Roo::Dict::get_property(message, Roo::keyword("type"))->str();

        if (type == "push")
        {
          mode_stack.update_state(active_mode->state);
          auto overrides_val =
            Roo::Dict::get_property(message, Roo::keyword("overrides"));
          push_mode(Roo::Dict::get_property(message, Roo::keyword("mode"))->str(),
                    Roo::Dict::get_property(message, Roo::keyword("state")),
                    overrides_val ? overrides_val : Roo::Constant::NIL);
          active_frame_changed = true;
        }
        else if (type == "pop")
        {
          auto payload = Roo::Dict::get_property(message, Roo::keyword("payload"));
          pop_mode(payload ? payload : Roo::Constant::NIL);
          active_frame_changed = true;
        }
        else if (type == "focus")
        {
          auto target =
            resolve_target_view(Roo::Dict::get_property(message, KEYWORD__TARGET));
          if (focus_target_view(active_mode, focus_state, target))
          {
            focus_changed = true;
          }
        }
        else if (type == "blur")
        {
          auto target =
            resolve_target_view(Roo::Dict::get_property(message, KEYWORD__TARGET));
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
        else if (type == "theme")
        {
          auto theme_val = Roo::Dict::get_property(message, KEYWORD__THEME);
          auto theme_names = Script::parse_theme_names(theme_val, "set-theme! theme");
          auto variant = Script::parse_theme_variant(
            Roo::Dict::get_property(message, KEYWORD__THEME_VARIANT),
            "set-theme! theme variant");
          set_application_theme(
            theme_names.empty() ? std::nullopt : std::make_optional(std::move(theme_names)),
            variant);
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

    if (active_frame_changed && this->hook_args.events)
    {
      this->hook_args.events->clear_transients();
    }

    return active_frame_changed;
  }

  void Session::render_mode()
  {
    PIXILS_BENCHMARK_COUNT(runtime_render_mode_calls);
    PIXILS_BENCHMARK_TIME_BLOCK(runtime_render_time_ns);

    auto render_stack = mode_stack.get_render_stack();
    Rect full = {0, 0, render_ctx.buffer_dim.w, render_ctx.buffer_dim.h};

    /**
     * render_stack is top-first: [0]=top, [1]=just below, ..., [n-1]=bottom.
     * Render from bottom up, skipping index 0 (active_mode, rendered last).
     */
    for (size_t i = render_stack.size() - 1; i > 0; i--)
    {
      size_t ctx_idx = ctx_stack.size() - i;
      size_t frame_meta_idx = frame_metadata.size() - 1 - i;
      if (render_stack[i - 1].first->composition.interaction_refresh)
      {
        UI::MouseState empty_mouse_state;
        UI::FocusState empty_focus_state;
        Pixils::UI::refresh_view_interaction_visual_state_tree(ctx_stack[ctx_idx],
                                                               empty_mouse_state,
                                                               empty_focus_state,
                                                               current_mouse_pos(hook_args));
        mode_stack.update_state(ctx_stack[ctx_idx]->state, render_stack.size() - i);
      }
      if (Pixils::UI::layout_view_tree(ctx_stack[ctx_idx],
                                       full,
                                       roo_runtime,
                                       hook_args.render_args[1]))
      {
        mode_stack.update_state(ctx_stack[ctx_idx]->state, render_stack.size() - i);
      }
      if (frame_meta_idx < frame_metadata.size() &&
          apply_overlay_placement(ctx_stack[ctx_idx], frame_metadata[frame_meta_idx], full))
      {
        mode_stack.update_state(ctx_stack[ctx_idx]->state, render_stack.size() - i);
      }
      Pixils::UI::render_view(render_ctx,
                              roo_runtime,
                              hook_args.render_args[1],
                              ctx_stack[ctx_idx]);
    }

    if (Pixils::UI::layout_view_tree(active_mode,
                                     full,
                                     roo_runtime,
                                     hook_args.render_args[1]))
    {
      mode_stack.update_state(active_mode->state);
      hook_args.update_state(active_mode->state);
    }
    if (!frame_metadata.empty() &&
        apply_overlay_placement(active_mode, frame_metadata.back(), full))
    {
      mode_stack.update_state(active_mode->state);
      hook_args.update_state(active_mode->state);
    }
    Pixils::UI::render_view(render_ctx,
                            roo_runtime,
                            hook_args.render_args[1],
                            active_mode);
  }
} // namespace Pixils::Runtime
