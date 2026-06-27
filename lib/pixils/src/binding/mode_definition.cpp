
#include "pixils/binding/mode_definition.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/binding/ui/style/style_host_type.h>
#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/runtime/state.h>
#include <pixils/ui/style.h>

#include <algorithm>
#include <roo/context.h>
#include <roo/exception.h>
#include <roo/host/schema.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>
#include <memory>
#include <unordered_map>

namespace Pixils::Script
{
  namespace
  {
    void append_class_names(std::vector<std::string>& target,
                            const std::vector<std::string>& classes)
    {
      for (const auto& class_name : classes)
      {
        if (std::find(target.begin(), target.end(), class_name) == target.end())
        {
          target.push_back(class_name);
        }
      }
    }

    /**
     * Evaluate a hook value at definition time. LIST-type values are evaluated
     * (legacy compatibility); everything else passes through as-is. Symbol
     * resolution happens separately at activation time via resolve_hook in
     * session.cpp.
     */
    Roo::sptr_val eval_hook(Roo::Context& ctx, const Roo::sptr_val& val)
    {
      if (val && val->type == Roo::Value::Type::LIST)
      {
        return ctx.eval(val->to_string());
      }
      return val ? val : Roo::Constant::NIL;
    }

    UI::DragStartMode parse_drag_start_mode(const Roo::sptr_val& value)
    {
      if (!value || value->type != Roo::Value::Type::KEYWORD)
        throw Roo::TypeError("Mode :drag :start :mode must be a keyword");

      if (value->str() == "motion") return UI::DragStartMode::MOTION;
      if (value->str() == "immediate") return UI::DragStartMode::IMMEDIATE;
      if (value->str() == "threshold") return UI::DragStartMode::THRESHOLD;
      throw Roo::TypeError("unknown drag start mode :" + value->str());
    }

    UI::DragStartPolicy parse_drag_start_policy(Roo::Context& ctx,
                                                const Roo::sptr_val& value)
    {
      UI::DragStartPolicy policy;
      if (!value || value->type == Roo::Value::Type::NIL) return policy;

      if (value->type == Roo::Value::Type::KEYWORD)
      {
        policy.mode = parse_drag_start_mode(value);
        if (policy.mode == UI::DragStartMode::THRESHOLD) policy.distance = 3;
        return policy;
      }

      if (value->type != Roo::Value::Type::MAP)
        throw Roo::TypeError("Mode :drag :start must be a keyword or map");

      static Roo::MapSchema start_schema(
        {},
        {{"mode", &Roo::Type::KEYWORD}, {"distance", &Roo::Type::NUMBER}});
      auto opts = start_schema.bind(ctx, *value);
      if (opts.contains("mode")) policy.mode = parse_drag_start_mode(opts.val("mode"));
      if (opts.contains("distance"))
      {
        policy.distance = std::max(0, opts.i32("distance"));
        if (!opts.contains("mode")) policy.mode = UI::DragStartMode::THRESHOLD;
      }
      return policy;
    }

    std::optional<UI::DragPolicy> parse_drag_policy(Roo::Context& ctx,
                                                    const Roo::sptr_val& value)
    {
      if (!value || value->type == Roo::Value::Type::NIL) return std::nullopt;
      if (value->type != Roo::Value::Type::MAP)
        throw Roo::TypeError("Mode :drag must be a map");

      static Roo::MapSchema drag_schema({},
                                           {{"button", &Roo::Type::KEYWORD},
                                            {"start", &Roo::Type::ANY},
                                            {"payload", &Roo::Type::ANY}});

      UI::DragPolicy policy;
      auto opts = drag_schema.bind(ctx, *value);
      if (opts.contains("button"))
        policy.button = UI::mouse_button_from_name(opts.str("button"));
      if (policy.button == UI::MouseButton::NONE)
        throw Roo::TypeError("Mode :drag :button must be :left, :right, or :middle");
      if (opts.contains("start"))
        policy.start = parse_drag_start_policy(ctx, opts.val("start"));
      if (opts.contains("payload")) policy.payload = eval_hook(ctx, opts.val("payload"));
      return policy;
    }

  } // namespace

  std::vector<std::string> parse_mode_classes(const Roo::sptr_val& class_val)
  {
    std::vector<std::string> classes;
    if (!class_val || class_val->type == Roo::Value::Type::NIL) return classes;

    auto parse_one = [&](const Roo::sptr_val& value)
    {
      if (value->type != Roo::Value::Type::KEYWORD)
        throw Roo::TypeError("Mode :class entries must be keywords");
      classes.push_back(value->str());
    };

    if (class_val->type == Roo::Value::Type::KEYWORD)
    {
      parse_one(class_val);
      return classes;
    }

    if (class_val->type == Roo::Value::Type::VECTOR)
    {
      for (const auto& child : Roo::get_children(*class_val))
      {
        parse_one(child);
      }
      return classes;
    }

    throw Roo::TypeError("Mode :class must be a keyword or vector of keywords");
  }

  std::vector<std::string> parse_theme_names(const Roo::sptr_val& theme_val,
                                             const std::string& context)
  {
    std::vector<std::string> names;
    if (!theme_val || theme_val->type == Roo::Value::Type::NIL) return names;

    auto parse_one = [&](const Roo::sptr_val& value)
    {
      if (value->type != Roo::Value::Type::SYMBOL)
      {
        throw Roo::TypeError(context + " entries must be symbols");
      }
      names.push_back(value->str());
    };

    if (theme_val->type == Roo::Value::Type::SYMBOL)
    {
      parse_one(theme_val);
      return names;
    }

    if (theme_val->type == Roo::Value::Type::VECTOR)
    {
      for (const auto& child : Roo::get_children(*theme_val))
      {
        parse_one(child);
      }
      return names;
    }

    throw Roo::TypeError(context + " must be a symbol or vector of symbols");
  }

  std::optional<std::string> parse_theme_variant(const Roo::sptr_val& variant_val,
                                                 const std::string& context)
  {
    if (!variant_val || variant_val->type == Roo::Value::Type::NIL) return std::nullopt;
    if (variant_val->type != Roo::Value::Type::KEYWORD &&
        variant_val->type != Roo::Value::Type::SYMBOL)
    {
      throw Roo::TypeError(context + " must be a keyword or symbol");
    }
    return variant_val->str();
  }

  void append_mode_style_layer(Roo::Context& ctx,
                               Runtime::Mode& mode,
                               const Roo::sptr_val& style_val)
  {
    if (!style_val || style_val->type == Roo::Value::Type::NIL) return;

    auto append_materialized_style = [&](const UI::Style& style)
    {
      if (!mode.style_layers.empty())
      {
        mode.style_layers.push_back(Runtime::StyleLayer{.style = style});
        if (!mode.style) mode.style = UI::Style{};
        return;
      }

      if (!mode.style)
      {
        mode.style = style;
      }
      else
      {
        UI::apply_style_variant(*mode.style, style);
      }
    };

    if (HostType::STYLE.is_type_of(*style_val))
    {
      auto style = Roo::obj<UI::Style>(*style_val);
      append_materialized_style(style);
      return;
    }

    if (contains_theme_var_ref(style_val))
    {
      if (mode.style && mode.style_layers.empty())
      {
        mode.style_layers.push_back(Runtime::StyleLayer{.style = *mode.style});
        mode.style = UI::Style{};
      }
      mode.style_layers.push_back(Runtime::StyleLayer{.source = style_val});
      if (!mode.style) mode.style = UI::Style{};
      return;
    }

    auto mutable_style_val = style_val;
    auto coercion = HostType::STYLE.coerce(ctx, mutable_style_val);
    if (!coercion.success)
    {
      throw Roo::TypeError("Mode :style must be a style map or style. Got: " +
                              style_val->to_string());
    }

    auto style = Roo::obj<UI::Style>(*coercion.result);
    append_materialized_style(style);
  }

  std::vector<Runtime::ChildSlot> parse_child_slots(Roo::Context& ctx,
                                                    const Roo::sptr_val& children_val)
  {
    static Roo::MapSchema child_schema({},
                                          {{"mode", &Roo::Type::SYMBOL},
                                           {"id", &Roo::Type::ANY},
                                           {"state", &Roo::Type::ANY}});

    std::unordered_map<std::string, int> name_counts;
    std::vector<Runtime::ChildSlot> slots;

    size_t n = Roo::count(*children_val);
    for (size_t i = 0; i < n; i++)
    {
      auto child_entry = Roo::get_child(*children_val, i);
      auto child_opts = child_schema.bind(ctx, *child_entry);

      Runtime::ChildSlot slot;
      if (child_opts.contains("mode"))
      {
        slot.mode_name = child_opts.val("mode")->str();
        slot.overrides = child_entry;
      }
      else
      {
        slot.anonymous_mode =
          std::make_shared<Runtime::Mode>(build_mode_from_definition(ctx, child_entry));
      }

      if (child_opts.contains("id"))
      {
        slot.id = child_opts.val("id")->str();
      }
      else
      {
        std::string base_name = slot.mode_name;
        if (base_name.empty() && slot.anonymous_mode && !slot.anonymous_mode->name.empty())
        {
          base_name = slot.anonymous_mode->name;
        }
        if (base_name.empty()) base_name = "anonymous";

        int idx = name_counts[base_name]++;
        slot.id = base_name + "-" + std::to_string(idx);
      }

      if (slot.anonymous_mode && slot.anonymous_mode->name.empty())
        slot.anonymous_mode->name = slot.id;

      auto [binding, initial] = Runtime::parse_state_binding(child_opts.val("state"));
      slot.state_binding = binding;
      slot.initial_state = initial;
      slots.push_back(std::move(slot));
    }
    return slots;
  }

  Runtime::Mode build_mode_from_definition(Roo::Context& ctx,
                                           const Roo::sptr_val& definition_map,
                                           const Runtime::Mode* base)
  {
    static Roo::MapSchema mode_schema({},
                                         {{"name", &Roo::Type::STRING},
                                          {"extend", &Roo::Type::SYMBOL_VALUE},
                                          {"init", &Roo::Type::ANY},
                                          {"update", &Roo::Type::ANY},
                                          {"after-layout", &Roo::Type::ANY},
                                          {"content-size", &Roo::Type::ANY},
                                          {"render", &Roo::Type::ANY},
                                          {"action-map", &Roo::Type::ANY},
                                          {"on-key-down", &Roo::Type::ANY},
                                          {"on-key-held", &Roo::Type::ANY},
                                          {"on-key-up", &Roo::Type::ANY},
                                          {"on-mouse-down", &Roo::Type::ANY},
                                          {"on-mouse-up", &Roo::Type::ANY},
                                          {"on-click", &Roo::Type::ANY},
                                          {"on-double-click", &Roo::Type::ANY},
                                          {"on-mouse-enter", &Roo::Type::ANY},
                                          {"on-mouse-leave", &Roo::Type::ANY},
                                          {"on-mouse-motion", &Roo::Type::ANY},
                                          {"on-drag-start", &Roo::Type::ANY},
                                          {"on-drag", &Roo::Type::ANY},
                                          {"on-drag-end", &Roo::Type::ANY},
                                          {"on-drop", &Roo::Type::ANY},
                                          {"on", &Roo::Type::MAP},
                                          {"compose", &HostType::MODE_COMPOSITION},
                                          {"resources", &HostType::RESOURCE_DEPENDENCIES},
                                          {"drag", &Roo::Type::MAP},
                                          {"style", &Roo::Type::ANY},
                                          {"class", &Roo::Type::ANY},
                                          {"focusable", &Roo::Type::BOOL},
                                          {"theme", &Roo::Type::ANY},
                                          {"theme-variant", &Roo::Type::ANY},
                                          {"children", &Roo::Type::ANY}});

    auto opts = mode_schema.bind(ctx, *definition_map);

    Runtime::Mode mode;

    if (opts.contains("extend"))
    {
      auto extends_name = opts.str("extend", "");
      auto modes = ctx.lookup(ID__PIXILS__MODES);
      auto base_val = Roo::Dict::get_property(modes, Roo::symbol(extends_name));
      if (!base_val || base_val->type == Roo::Value::Type::NIL)
        throw Roo::InvocationException("defmode :extends - unknown base mode '" +
                                          extends_name + "'");
      mode = Roo::obj<Runtime::Mode>(*base_val);
    }
    else if (base)
    {
      mode = *base;
    }

    if (opts.contains("name")) mode.name = opts.str("name", "");
    if (!mode.name.empty())
    {
      if (mode.selector_modes.empty())
      {
        mode.selector_modes.push_back(mode.name);
      }
      else
      {
        mode.selector_modes.insert(mode.selector_modes.begin(), mode.name);
      }
    }

    auto apply_hook = [&](Roo::sptr_val& field, const char* key)
    {
      if (opts.contains(key))
      {
        field = eval_hook(ctx, opts.val(key));
      }
    };

    apply_hook(mode.init, "init");
    apply_hook(mode.update, "update");
    apply_hook(mode.after_layout, "after-layout");
    apply_hook(mode.content_size, "content-size");
    apply_hook(mode.render, "render");
    apply_hook(mode.action_map, "action-map");
    apply_hook(mode.on_key_down, "on-key-down");
    apply_hook(mode.on_key_held, "on-key-held");
    apply_hook(mode.on_key_up, "on-key-up");
    apply_hook(mode.on_mouse_down, "on-mouse-down");
    apply_hook(mode.on_mouse_up, "on-mouse-up");
    apply_hook(mode.on_click, "on-click");
    apply_hook(mode.on_double_click, "on-double-click");
    apply_hook(mode.on_mouse_enter, "on-mouse-enter");
    apply_hook(mode.on_mouse_leave, "on-mouse-leave");
    apply_hook(mode.on_mouse_motion, "on-mouse-motion");
    apply_hook(mode.on_drag_start, "on-drag-start");
    apply_hook(mode.on_drag, "on-drag");
    apply_hook(mode.on_drag_end, "on-drag-end");
    apply_hook(mode.on_drop, "on-drop");

    if (opts.contains("on"))
    {
      auto on_val = opts.val("on");
      if (on_val->type == Roo::Value::Type::MAP)
      {
        for (auto& key : Roo::Dict::keys(*on_val))
          mode.event_handlers[key->str()] = Roo::Dict::get_property(on_val, key);
      }
    }

    if (opts.contains("compose"))
      mode.composition = opts.obj<Runtime::ModeComposition>("compose");

    if (opts.contains("resources"))
    {
      auto res = opts.optional_obj<Runtime::ResourceDependencies>("resources");
      if (res.has_value()) mode.resources = *res;
    }

    if (opts.contains("drag")) mode.drag = parse_drag_policy(ctx, opts.val("drag"));

    if (opts.contains("style"))
    {
      append_mode_style_layer(ctx, mode, opts.val("style"));
    }
    if (opts.contains("class"))
    {
      append_class_names(mode.class_names, parse_mode_classes(opts.val("class")));
    }
    if (opts.contains("focusable"))
    {
      mode.focusable = opts.boolean("focusable");
    }
    if (opts.contains("theme"))
    {
      auto theme_names = parse_theme_names(opts.val("theme"), "Mode :theme");
      mode.theme =
        theme_names.empty() ? std::nullopt : std::make_optional(std::move(theme_names));
    }
    if (opts.contains("theme-variant"))
    {
      mode.theme_variant =
        parse_theme_variant(opts.val("theme-variant"), "Mode :theme-variant");
    }

    if (opts.contains("children"))
    {
      auto children_val = opts.val("children");
      if (children_val->type != Roo::Value::Type::NIL)
      {
        mode.children = parse_child_slots(ctx, children_val);
      }
    }

    return mode;
  }

} // namespace Pixils::Script
