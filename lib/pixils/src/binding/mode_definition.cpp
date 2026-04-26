
#include "pixils/binding/mode_definition.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/resource_namespace.h>
#include <pixils/binding/style_namespace.h>
#include <pixils/runtime/state.h>
#include <pixils/ui/style.h>

#include <iostream>
#include <lisple/context.h>
#include <lisple/exception.h>
#include <lisple/host.h>
#include <lisple/host/schema.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/seq.h>
#include <lisple/runtime/value.h>
#include <unordered_map>

namespace Pixils::Script
{
  namespace
  {
    /**
     * Evaluate a hook value at definition time. LIST-type values are evaluated
     * (legacy compatibility); everything else passes through as-is. Symbol
     * resolution happens separately at activation time via resolve_hook in
     * session.cpp.
     */
    Lisple::sptr_rtval eval_hook(Lisple::Context& ctx, const Lisple::sptr_rtval& val)
    {
      if (val && val->type == Lisple::RTValue::Type::LIST)
      {
        return ctx.eval(val->to_string());
      }
      return val ? val : Lisple::Constant::NIL;
    }

  } // namespace

  std::vector<Runtime::ChildSlot> parse_child_slots(Lisple::Context& ctx,
                                                    const Lisple::sptr_rtval& children_val)
  {
    static Lisple::MapSchema child_schema(
      {{"mode", &Lisple::Type::SYMBOL}},
      {{"id", &Lisple::Type::ANY}, {"state", &Lisple::Type::ANY}});

    std::unordered_map<std::string, int> name_counts;
    std::vector<Runtime::ChildSlot> slots;

    size_t n = Lisple::count(*children_val);
    for (size_t i = 0; i < n; i++)
    {
      auto child_entry = Lisple::get_child(*children_val, i);
      auto child_opts = child_schema.bind(ctx, *child_entry);

      Runtime::ChildSlot slot;
      slot.mode_name = child_opts.val("mode")->str();

      if (child_opts.contains("id"))
        slot.id = child_opts.val("id")->str();
      else
      {
        int idx = name_counts[slot.mode_name]++;
        slot.id = slot.mode_name + "-" + std::to_string(idx);
      }

      auto [binding, initial] = Runtime::parse_state_binding(child_opts.val("state"));
      slot.state_binding = binding;
      slot.initial_state = initial;
      slot.overrides = child_entry;
      slots.push_back(std::move(slot));
    }
    return slots;
  }

  Runtime::Mode build_mode_from_definition(Lisple::Context& ctx,
                                           const Lisple::sptr_rtval& definition_map,
                                           const Runtime::Mode* base)
  {
    static Lisple::MapSchema mode_schema({},
                                         {{"name", &Lisple::Type::STRING},
                                          {"extend", &Lisple::Type::SYMBOL_VALUE},
                                          {"init", &Lisple::Type::ANY},
                                          {"update", &Lisple::Type::ANY},
                                          {"render", &Lisple::Type::ANY},
                                          {"on-mouse-down", &Lisple::Type::ANY},
                                          {"on-mouse-up", &Lisple::Type::ANY},
                                          {"on-click", &Lisple::Type::ANY},
                                          {"on-mouse-enter", &Lisple::Type::ANY},
                                          {"on-mouse-leave", &Lisple::Type::ANY},
                                          {"on", &Lisple::Type::MAP},
                                          {"compose", &HostType::MODE_COMPOSITION},
                                          {"resources", &HostType::RESOURCE_DEPENDENCIES},
                                          {"style", &HostType::STYLE},
                                          {"children", &Lisple::Type::ANY}});

    auto opts = mode_schema.bind(ctx, *definition_map);

    Runtime::Mode mode;

    if (opts.contains("extend"))
    {
      auto extends_name = opts.str("extend", "");
      auto modes = ctx.lookup_value(ID__PIXILS__MODES);
      auto base_val =
        Lisple::Dict::get_property(modes, Lisple::RTValue::symbol(extends_name));
      if (!base_val || base_val->type == Lisple::RTValue::Type::NIL)
        throw Lisple::InvocationException("defmode :extends - unknown base mode '" +
                                          extends_name + "'");
      mode = Lisple::obj<Runtime::Mode>(*base_val);
    }
    else if (base)
    {
      mode = *base;
    }

    if (opts.contains("name")) mode.name = opts.str("name", "");

    auto apply_hook = [&](Lisple::sptr_rtval& field, const char* key)
    {
      if (opts.contains(key)) field = eval_hook(ctx, opts.val(key));
    };

    apply_hook(mode.init, "init");
    apply_hook(mode.update, "update");
    apply_hook(mode.render, "render");
    apply_hook(mode.on_mouse_down, "on-mouse-down");
    apply_hook(mode.on_mouse_up, "on-mouse-up");
    apply_hook(mode.on_click, "on-click");
    apply_hook(mode.on_mouse_enter, "on-mouse-enter");
    apply_hook(mode.on_mouse_leave, "on-mouse-leave");

    if (opts.contains("on"))
    {
      auto on_val = opts.val("on");
      if (on_val->type == Lisple::RTValue::Type::MAP)
      {
        for (auto& key : Lisple::Dict::keys(*on_val))
          mode.event_handlers[key->str()] = Lisple::Dict::get_property(on_val, key);
      }
    }

    if (opts.contains("compose"))
      mode.composition = opts.obj<Runtime::ModeComposition>("compose");

    if (opts.contains("resources"))
    {
      auto res = opts.optional_obj<Runtime::ResourceDependencies>("resources");
      if (res.has_value()) mode.resources = *res;
    }

    mode.style = opts.optional_obj<UI::Style>("style");

    if (opts.contains("children"))
    {
      auto children_val = opts.val("children");
      if (children_val->type != Lisple::RTValue::Type::NIL)
        mode.children = parse_child_slots(ctx, children_val);
    }

    return mode;
  }

} // namespace Pixils::Script
