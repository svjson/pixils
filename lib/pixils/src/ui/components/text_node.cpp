#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>
#include <pixils/text.h>
#include <pixils/ui/components/text_node.h>
#include <pixils/ui/style.h>
#include <pixils/ui/text_style.h>

#include <roo/exec.h>
#include <roo/host/object.h>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/value.h>

namespace Pixils::UI::Components
{
  namespace
  {
    std::string text_node_value(const Roo::sptr_val& state)
    {
      if (!state || state->type == Roo::Value::Type::NIL) return "";
      if (state->type == Roo::Value::Type::STRING) return state->str();
      if (state->type != Roo::Value::Type::MAP) return state->to_string();

      auto value = Roo::Dict::get_property(state, Roo::keyword("value"));
      if (!value || value->type == Roo::Value::Type::NIL) return "";
      if (value->type != Roo::Value::Type::STRING) return value->to_string();
      return value->str();
    }

    namespace Function
    {
      FUNC(TextNodeContentSize, text_node_content_size);
      FUNC(TextNodeRender, text_node_render);

      FUNC_IMPL(TextNodeContentSize,
                SIG((FN_ARGS((&Roo::Type::ANY), (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&TextNodeContentSize::exec_text_node_content_size))));

      EXEC_BODY(TextNodeContentSize, exec_text_node_content_size)
      {
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[1]);
        if (!hook_ctx.current_view) return Roo::Constant::NIL;

        RenderContext& rc =
          Roo::obj<RenderContext>(*ctx.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
        const Runtime::View& view = *hook_ctx.current_view;
        auto text_op = TextStyle::make_render_op(rc, view.effective_style);
        if (!text_op) return Roo::Constant::NIL;

        auto layout = Text::layout_text(rc,
                                        *text_op,
                                        text_node_value(args[0]),
                                        TextStyle::wrap_mode(view.effective_style),
                                        hook_ctx.available_width);
        return Script::DimensionAdapter::make_unique(layout.size.w, layout.size.h);
      }

      FUNC_IMPL(TextNodeRender,
                SIG((FN_ARGS((&Roo::Type::ANY), (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&TextNodeRender::exec_text_node_render))));

      EXEC_BODY(TextNodeRender, exec_text_node_render)
      {
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[1]);
        if (!hook_ctx.current_view) return Roo::Constant::NIL;

        RenderContext& rc =
          Roo::obj<RenderContext>(*ctx.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
        const Runtime::View& view = *hook_ctx.current_view;

        auto text_op = TextStyle::make_render_op(rc,
                                                 view.effective_style,
                                                 TextStyle::color(view.effective_style));
        if (!text_op) return Roo::Constant::NIL;

        const Rect content_rect = view.effective_style.content_rect(view.bounds);
        auto layout = Text::layout_text(rc,
                                        *text_op,
                                        text_node_value(args[0]),
                                        TextStyle::wrap_mode(view.effective_style),
                                        content_rect.w);

        for (size_t i = 0; i < layout.lines.size(); i++)
        {
          int x = 0;
          switch (TextStyle::alignment(view.effective_style))
          {
          case Text::Alignment::CENTER:
            x += (content_rect.w - layout.lines[i].width) / 2;
            break;
          case Text::Alignment::RIGHT:
            x += content_rect.w - layout.lines[i].width;
            break;
          default:
            break;
          }

          int y =
            static_cast<int>(i) * (layout.size.h / static_cast<int>(layout.lines.size()));
          Text::render_layout_line(rc, *text_op, layout.lines[i], x, y);
        }
        return Roo::Constant::NIL;
      }
    } // namespace Function

    Runtime::Mode make_text_node_component_mode()
    {
      Runtime::Mode mode;
      mode.name = "ui/text";
      mode.selector_modes.push_back(mode.name);
      mode.content_size = Function::TextNodeContentSize::make();
      mode.render = Function::TextNodeRender::make();
      return mode;
    }
  } // namespace

  void register_text_node_component(Roo::Runtime& runtime)
  {
    auto modes = runtime.lookup(Script::ID__PIXILS__MODES);
    Roo::Dict::set_property(
      modes,
      Roo::symbol("ui/text"),
      Script::ModeAdapter::make_unique(make_text_node_component_mode()));
  }
} // namespace Pixils::UI::Components
