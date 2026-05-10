#include <pixils/binding/pixils_namespace.h>
#include <pixils/context.h>
#include <pixils/font_registry.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>
#include <pixils/text.h>
#include <pixils/ui/components/text_node.h>
#include <pixils/ui/style.h>

#include <lisple/exec.h>
#include <lisple/host/object.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <lisple/runtime/value.h>

namespace Pixils::UI::Components
{
  namespace
  {
    std::string text_node_value(const Lisple::sptr_rtval& state)
    {
      if (!state || state->type == Lisple::RTValue::Type::NIL) return "";
      if (state->type == Lisple::RTValue::Type::STRING) return state->str();
      if (state->type != Lisple::RTValue::Type::MAP) return state->to_string();

      auto value = Lisple::Dict::get_property(state, Lisple::RTValue::keyword("value"));
      if (!value || value->type == Lisple::RTValue::Type::NIL) return "";
      if (value->type != Lisple::RTValue::Type::STRING) return value->to_string();
      return value->str();
    }

    int text_style_scale(const Style& style)
    {
      if (style.text && style.text->scale) return *style.text->scale;
      return 1;
    }

    std::optional<Color> text_style_color(const Style& style)
    {
      if (style.text && style.text->color) return style.text->color;
      return std::nullopt;
    }

    std::string text_style_font_key(const Style& style)
    {
      if (style.text && style.text->font) return *style.text->font;
      return "font/console";
    }

    Text::Alignment text_style_alignment(const Style& style)
    {
      if (style.text && style.text->align) return *style.text->align;
      return Text::Alignment::LEFT;
    }

    Text::WrapMode text_style_wrap_mode(const Style& style)
    {
      if (!style.text || !style.text->wrap) return Text::WrapMode::WORD;
      return *style.text->wrap == Style::Text::Wrap::NONE ? Text::WrapMode::NONE
                                                          : Text::WrapMode::WORD;
    }

    namespace Function
    {
      FUNC(TextNodeContentSize, text_node_content_size);
      FUNC(TextNodeRender, text_node_render);

      FUNC_IMPL(TextNodeContentSize,
                SIG((FN_ARGS((&Lisple::Type::ANY),
                             (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&TextNodeContentSize::exec_text_node_content_size))));

      EXEC_BODY(TextNodeContentSize, exec_text_node_content_size)
      {
        HookContext& hook_ctx = Lisple::obj<HookContext>(*args[1]);
        if (!hook_ctx.current_view) return Lisple::Constant::NIL;

        RenderContext& rc =
          Lisple::obj<RenderContext>(*ctx.lookup_value(Script::ID__PIXILS__RENDER_CONTEXT));
        const Runtime::View& view = *hook_ctx.current_view;
        auto text_op = Text::make_text_render_op(rc,
                                                 text_style_font_key(view.effective_style),
                                                 text_style_scale(view.effective_style));
        if (!text_op) return Lisple::Constant::NIL;

        auto layout = Text::layout_text(rc,
                                        *text_op,
                                        text_node_value(args[0]),
                                        text_style_wrap_mode(view.effective_style),
                                        hook_ctx.available_width);
        return Script::DimensionAdapter::make_unique(layout.size.w, layout.size.h);
      }

      FUNC_IMPL(TextNodeRender,
                SIG((FN_ARGS((&Lisple::Type::ANY),
                             (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&TextNodeRender::exec_text_node_render))));

      EXEC_BODY(TextNodeRender, exec_text_node_render)
      {
        HookContext& hook_ctx = Lisple::obj<HookContext>(*args[1]);
        if (!hook_ctx.current_view) return Lisple::Constant::NIL;

        RenderContext& rc =
          Lisple::obj<RenderContext>(*ctx.lookup_value(Script::ID__PIXILS__RENDER_CONTEXT));
        const Runtime::View& view = *hook_ctx.current_view;

        auto text_op = Text::make_text_render_op(rc,
                                                 text_style_font_key(view.effective_style),
                                                 text_style_scale(view.effective_style),
                                                 text_style_color(view.effective_style));
        if (!text_op) return Lisple::Constant::NIL;

        const Rect content_rect = view.effective_style.content_rect(view.bounds);
        auto layout = Text::layout_text(rc,
                                        *text_op,
                                        text_node_value(args[0]),
                                        text_style_wrap_mode(view.effective_style),
                                        content_rect.w);

        Text::Cursor cursor(*text_op->renderer,
                            text_op->color,
                            text_op->renderer->get_line_height());
        cursor.set_alignment(text_style_alignment(view.effective_style));

        switch (cursor.get_alignment())
        {
        case Text::Alignment::CENTER:
          cursor.move_to(content_rect.w / 2, 0);
          break;
        case Text::Alignment::RIGHT:
          cursor.move_to(content_rect.w, 0);
          break;
        default:
          cursor.move_to(0, 0);
          break;
        }

        for (size_t i = 0; i < layout.lines.size(); i++)
        {
          if (i + 1 < layout.lines.size())
          {
            cursor.println(rc, layout.lines[i].text);
            continue;
          }

          cursor.print(rc, layout.lines[i].text);
        }
        return Lisple::Constant::NIL;
      }
    } // namespace Function

    Runtime::Mode make_text_node_component_mode()
    {
      Runtime::Mode mode;
      mode.name = "text-node";
      mode.content_size = Function::TextNodeContentSize::make();
      mode.render = Function::TextNodeRender::make();
      return mode;
    }
  } // namespace

  void register_text_node_component(Lisple::Runtime& runtime)
  {
    auto modes = runtime.lookup_value(Script::ID__PIXILS__MODES);
    Lisple::Dict::set_property(
      modes,
      Lisple::RTValue::symbol("text-node"),
      Script::ModeAdapter::make_unique(make_text_node_component_mode()));
  }
} // namespace Pixils::UI::Components
