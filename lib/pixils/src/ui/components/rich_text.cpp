#include <pixils/binding/mode_definition.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/rect_namespace.h>
#include <pixils/binding/ui/style/style_host_type.h>
#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/binding/ui/ui_host_type.h>
#include <pixils/context.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/mode.h>
#include <pixils/runtime/view.h>
#include <pixils/text.h>
#include <pixils/ui/components/rich_text.h>
#include <pixils/ui/event.h>
#include <pixils/ui/style.h>
#include <pixils/ui/text_style.h>
#include <pixils/ui/theme.h>

#include <algorithm>
#include <optional>
#include <roo/exception.h>
#include <roo/exec.h>
#include <roo/host/object.h>
#include <roo/runtime.h>
#include <roo/runtime/dict.h>
#include <roo/runtime/seq.h>
#include <roo/runtime/value.h>
#include <string>
#include <vector>

namespace Pixils::UI::Components
{
  namespace
  {
    struct RichRun
    {
      std::string text;
      std::size_t index = 0;
      bool marked = false;
      bool interactive = false;
      Roo::sptr_val source = Roo::Constant::NIL;
      Roo::sptr_val value = Roo::Constant::NIL;
      std::optional<Style> style = std::nullopt;
      std::vector<std::string> class_names;
    };

    struct RichToken
    {
      std::string text;
      std::size_t run_index = 0;
      bool marked = false;
      bool interactive = false;
      bool whitespace = false;
      std::string run_text;
      Roo::sptr_val source = Roo::Constant::NIL;
      Roo::sptr_val value = Roo::Constant::NIL;
      std::optional<Style> style = std::nullopt;
      int width = 0;
    };

    struct RichLine
    {
      std::vector<RichToken> tokens;
      int width = 0;
    };

    struct RichTextLayout
    {
      std::vector<RichLine> lines;
      Dimension size = {0, 0};
      int line_height = 0;
    };

    struct HitRun
    {
      std::size_t index = 0;
      std::string text;
      Roo::sptr_val source = Roo::Constant::NIL;
      Roo::sptr_val value = Roo::Constant::NIL;
      Rect bounds = {0, 0, 0, 0};
    };

    Roo::sptr_val map_value(const Roo::sptr_val& value, const std::string& key)
    {
      if (!value || value->type != Roo::Value::Type::MAP) return Roo::Constant::NIL;
      auto property = Roo::Dict::get_property(value, Roo::keyword(key));
      return property ? property : Roo::Constant::NIL;
    }

    bool has_value(const Roo::sptr_val& value)
    {
      return value && value->type != Roo::Value::Type::NIL;
    }

    void append_unique_font_style(std::vector<Text::FontStyle>& out, Text::FontStyle style)
    {
      if (std::find(out.begin(), out.end(), style) == out.end())
      {
        out.push_back(style);
      }
    }

    std::optional<Text::FontStyle> parse_font_style_keyword(const Roo::sptr_val& value)
    {
      if (!value || value->type != Roo::Value::Type::KEYWORD) return std::nullopt;
      if (value->str() == "bold") return Text::FontStyle::BOLD;
      if (value->str() == "underline") return Text::FontStyle::UNDERLINE;
      return std::nullopt;
    }

    std::optional<std::vector<Text::FontStyle>> parse_font_styles(const Roo::sptr_val& value)
    {
      if (!has_value(value)) return std::nullopt;

      if (auto single = parse_font_style_keyword(value))
      {
        return std::vector<Text::FontStyle>{*single};
      }

      if (value->type != Roo::Value::Type::VECTOR)
      {
        throw Roo::TypeError("rich-text run :font-styles must be a keyword or vector");
      }

      std::vector<Text::FontStyle> styles;
      for (auto& child : Roo::get_children(*value))
      {
        auto parsed = parse_font_style_keyword(child);
        if (!parsed)
        {
          throw Roo::TypeError("unknown rich-text run font style: " + child->to_string());
        }
        styles.push_back(*parsed);
      }
      return styles;
    }

    Style style_from_font_styles(const std::vector<Text::FontStyle>& font_styles)
    {
      Style style;
      style.text = Style::Text{};
      style.text->font_styles = font_styles;
      return style;
    }

    std::optional<Style> coerce_run_style(Roo::Context& ctx,
                                          const Theme& theme,
                                          const Roo::sptr_val& value)
    {
      if (!has_value(value)) return std::nullopt;

      auto resolved = Script::resolve_theme_vars(theme, theme.selected_variant, value);
      if (!resolved) return std::nullopt;

      auto mutable_style = resolved;
      auto coercion = Script::HostType::STYLE.coerce(ctx, mutable_style);
      if (!coercion.success)
      {
        throw Roo::TypeError("Invalid rich-text run :style: " + resolved->to_string());
      }

      return Roo::obj<Style>(*coercion.result);
    }

    std::optional<Style> rich_run_style(Roo::Context& ctx,
                                        const Theme& theme,
                                        const Roo::sptr_val& entry,
                                        const std::vector<std::string>& class_names)
    {
      std::optional<Style> style = std::nullopt;

      if (!class_names.empty())
      {
        ThemeMatchContext match;
        match.mode_names = {"ui/rich-text-run"};
        match.class_names = class_names;
        match.state = entry;

        for (const Style* theme_style : theme.get_matching_styles(match))
        {
          if (!style) style = Style{};
          apply_style_variant(*style, *theme_style);
        }
      }

      if (auto inline_style = coerce_run_style(ctx, theme, map_value(entry, "style")))
      {
        if (!style) style = Style{};
        apply_style_variant(*style, *inline_style);
      }

      if (auto font_styles = parse_font_styles(map_value(entry, "font-styles")))
      {
        if (!style) style = Style{};
        apply_style_variant(*style, style_from_font_styles(*font_styles));
      }

      return style;
    }

    std::string rich_run_text(const Roo::sptr_val& value)
    {
      if (!has_value(value)) return "";
      if (value->type == Roo::Value::Type::STRING) return value->str();

      if (value->type == Roo::Value::Type::MAP)
      {
        auto text = map_value(value, "text");
        if (!has_value(text)) text = map_value(value, "label");
        if (!has_value(text)) text = map_value(value, "title");
        if (!has_value(text)) return "";
        return text->type == Roo::Value::Type::STRING ? text->str() : text->to_string();
      }

      return value->to_string();
    }

    Roo::sptr_val rich_text_source(const Roo::sptr_val& state)
    {
      if (!state || state->type == Roo::Value::Type::NIL) return Roo::Constant::NIL;
      if (state->type != Roo::Value::Type::MAP) return state;

      auto runs = map_value(state, "runs");
      if (has_value(runs)) return runs;

      auto content = map_value(state, "content");
      if (has_value(content)) return content;

      return map_value(state, "value");
    }

    std::vector<RichRun> rich_runs_from_state(Roo::Context& ctx,
                                              const Theme& theme,
                                              const Roo::sptr_val& state)
    {
      auto source = rich_text_source(state);
      if (!has_value(source)) return {};

      Roo::sptr_val_v entries;
      if (source->type == Roo::Value::Type::VECTOR || source->type == Roo::Value::Type::LIST)
      {
        entries = Roo::get_children(*source);
      }
      else
      {
        entries.push_back(source);
      }

      std::vector<RichRun> runs;
      runs.reserve(entries.size());
      for (std::size_t i = 0; i < entries.size(); i++)
      {
        RichRun run{.text = rich_run_text(entries[i]),
                    .index = i,
                    .source = entries[i],
                    .class_names = {}};
        if (entries[i] && entries[i]->type == Roo::Value::Type::MAP)
        {
          run.marked = Roo::is_truthy(*map_value(entries[i], "marked?"));
          run.value = map_value(entries[i], "value");
          if (!has_value(run.value)) run.value = map_value(entries[i], "id");
          if (!has_value(run.value)) run.value = map_value(entries[i], "href");
          run.interactive = has_value(run.value);
          run.class_names = Script::parse_mode_classes(map_value(entries[i], "class"));
          run.style = rich_run_style(ctx, theme, entries[i], run.class_names);
        }
        runs.push_back(run);
      }
      return runs;
    }

    Text::Renderer& token_renderer(Text::TextRenderOp& op, bool marked)
    {
      if (marked && op.inline_style && op.inline_style->enabled && op.inline_style->renderer)
      {
        return *op.inline_style->renderer;
      }
      return *op.renderer;
    }

    Style token_effective_style(const Style& base_style, const RichToken& token)
    {
      auto style = base_style;
      if (token.style) apply_style_variant(style, *token.style);
      return style;
    }

    void apply_marked_run_font_styles(Text::TextRenderOp& op,
                                      const RichToken& token,
                                      const Style& style)
    {
      if (!token.marked || !style.text || !style.text->font_styles) return;
      if (!op.inline_style || !op.inline_style->enabled) return;

      auto styles = op.inline_style->font_styles;
      for (auto font_style : *style.text->font_styles)
      {
        append_unique_font_style(styles, font_style);
      }
      op.inline_style->font_styles = styles;
    }

    std::optional<Text::TextRenderOp> token_render_op(RenderContext& rc,
                                                      const Style& base_style,
                                                      const RichToken& token,
                                                      bool render_color)
    {
      auto style = token_effective_style(base_style, token);
      auto op =
        TextStyle::make_render_op(rc,
                                  style,
                                  render_color ? TextStyle::color(style) : std::nullopt);
      if (op) apply_marked_run_font_styles(*op, token, style);
      return op;
    }

    Text::Renderer& token_renderer(RenderContext& rc,
                                   Text::TextRenderOp& base_op,
                                   const Style& base_style,
                                   const RichToken& token)
    {
      auto op = token_render_op(rc, base_style, token, false);
      if (op)
      {
        return token.marked && op->inline_style && op->inline_style->enabled &&
                   op->inline_style->renderer
                 ? *op->inline_style->renderer
                 : *op->renderer;
      }
      return token_renderer(base_op, token.marked);
    }

    bool is_rich_text_whitespace(char c)
    {
      return c == ' ' || c == '\t';
    }

    void append_rich_token(std::vector<RichToken>& tokens,
                           const RichRun& run,
                           const std::string& text,
                           bool whitespace)
    {
      if (text.empty()) return;
      tokens.push_back({.text = text,
                        .run_index = run.index,
                        .marked = run.marked,
                        .interactive = run.interactive,
                        .whitespace = whitespace,
                        .run_text = run.text,
                        .source = run.source,
                        .value = run.value,
                        .style = run.style});
    }

    std::vector<RichToken> tokenize_rich_runs(const std::vector<RichRun>& runs)
    {
      std::vector<RichToken> tokens;
      for (const auto& run : runs)
      {
        std::string current;
        bool current_whitespace = false;
        bool has_current = false;

        auto flush = [&]()
        {
          append_rich_token(tokens, run, current, current_whitespace);
          current.clear();
          has_current = false;
        };

        for (std::size_t i = 0; i < run.text.size(); i++)
        {
          char c = run.text.at(i);
          if (c == '\r')
          {
            if (i + 1 < run.text.size() && run.text.at(i + 1) == '\n') i++;
            flush();
            tokens.push_back({.text = "\n", .run_index = run.index, .run_text = run.text});
            continue;
          }

          if (c == '\n')
          {
            flush();
            tokens.push_back({.text = "\n", .run_index = run.index, .run_text = run.text});
            continue;
          }

          const bool whitespace = is_rich_text_whitespace(c);
          if (has_current && current_whitespace != whitespace) flush();
          current.push_back(c);
          current_whitespace = whitespace;
          has_current = true;
        }
        flush();
      }
      return tokens;
    }

    int measure_rich_tokens(RenderContext& rc,
                            Text::TextRenderOp& op,
                            const Style& base_style,
                            std::vector<RichToken>& tokens)
    {
      int line_height = 1;
      for (auto& token : tokens)
      {
        if (token.text == "\n") continue;
        auto size =
          token_renderer(rc, op, base_style, token).get_rendered_size(rc, token.text);
        token.width = size.w;
        line_height = std::max(line_height, size.h);
      }
      return line_height;
    }

    void append_token_to_line(RichLine& line, const RichToken& token)
    {
      line.tokens.push_back(token);
      line.width += token.width;
    }

    void append_line_to_layout(RichTextLayout& layout, RichLine& line)
    {
      layout.size.w = std::max(layout.size.w, line.width);
      layout.lines.push_back(line);
      line = RichLine{};
    }

    RichTextLayout layout_rich_text(RenderContext& rc,
                                    Text::TextRenderOp& op,
                                    const Style& base_style,
                                    const std::vector<RichRun>& runs,
                                    Text::WrapMode wrap_mode,
                                    std::optional<int> max_width)
    {
      RichTextLayout layout;
      const bool should_wrap =
        wrap_mode == Text::WrapMode::WORD && max_width && *max_width > 0;
      auto tokens = tokenize_rich_runs(runs);
      layout.line_height = measure_rich_tokens(rc, op, base_style, tokens);

      RichLine current;
      RichLine pending_whitespace;
      bool at_line_start = true;

      for (const auto& token : tokens)
      {
        if (token.text == "\n")
        {
          if (!current.tokens.empty())
            append_line_to_layout(layout, current);
          else
            layout.lines.push_back({});
          pending_whitespace = RichLine{};
          at_line_start = true;
          continue;
        }

        if (token.whitespace)
        {
          append_token_to_line(pending_whitespace, token);
          continue;
        }

        if (current.tokens.empty())
        {
          if (at_line_start)
          {
            for (const auto& pending : pending_whitespace.tokens)
              append_token_to_line(current, pending);
          }
          append_token_to_line(current, token);
          pending_whitespace = RichLine{};
          at_line_start = false;
          continue;
        }

        if (!should_wrap ||
            current.width + pending_whitespace.width + token.width <= *max_width)
        {
          for (const auto& pending : pending_whitespace.tokens)
            append_token_to_line(current, pending);
          append_token_to_line(current, token);
          pending_whitespace = RichLine{};
          continue;
        }

        append_line_to_layout(layout, current);
        append_token_to_line(current, token);
        pending_whitespace = RichLine{};
        at_line_start = false;
      }

      if (!pending_whitespace.tokens.empty())
      {
        for (const auto& pending : pending_whitespace.tokens)
          append_token_to_line(current, pending);
      }

      if (!current.tokens.empty() || layout.lines.empty())
        append_line_to_layout(layout, current);
      layout.size.h = layout.line_height * static_cast<int>(layout.lines.size());
      return layout;
    }

    int line_start_x(const RichLine& line, Text::Alignment alignment, int available_width)
    {
      if (alignment == Text::Alignment::CENTER) return (available_width - line.width) / 2;
      if (alignment == Text::Alignment::RIGHT) return available_width - line.width;
      return 0;
    }

    Rect run_bounds_on_line(const RichLine& line,
                            std::size_t run_index,
                            int line_x,
                            int line_y,
                            int line_height)
    {
      bool found = false;
      Rect bounds{0, line_y, 0, line_height};
      int x = line_x;
      for (const auto& token : line.tokens)
      {
        if (token.run_index == run_index)
        {
          if (!found)
          {
            bounds.x = x;
            bounds.w = token.width;
            found = true;
          }
          else
          {
            bounds.w = (x + token.width) - bounds.x;
          }
        }
        x += token.width;
      }
      return bounds;
    }

    std::optional<HitRun> hit_rich_run(const RichTextLayout& layout,
                                       Text::Alignment alignment,
                                       int available_width,
                                       const Point& pos)
    {
      for (std::size_t line_index = 0; line_index < layout.lines.size(); line_index++)
      {
        const auto& line = layout.lines[line_index];
        int x = line_start_x(line, alignment, available_width);
        const int y = static_cast<int>(line_index) * layout.line_height;

        for (const auto& token : line.tokens)
        {
          const Rect token_rect{x, y, token.width, layout.line_height};
          if (token.interactive && token_rect.contains(pos))
            return HitRun{token.run_index,
                          token.run_text,
                          token.source,
                          token.value,
                          run_bounds_on_line(line,
                                             token.run_index,
                                             line_start_x(line, alignment, available_width),
                                             y,
                                             layout.line_height)};
          x += token.width;
        }
      }
      return std::nullopt;
    }

    Roo::sptr_val hit_payload(Runtime::View& view, const HitRun& hit)
    {
      return Roo::map({Roo::keyword("index"),
                       Roo::number(static_cast<int>(hit.index)),
                       Roo::keyword("text"),
                       Roo::string(hit.text),
                       Roo::keyword("run"),
                       hit.source,
                       Roo::keyword("value"),
                       hit.value,
                       Roo::keyword("anchor"),
                       Script::ViewAdapter::make_ref(view),
                       Roo::keyword("anchor-bounds"),
                       Script::RectAdapter::make_unique(hit.bounds)});
    }

    std::optional<int> hovered_run_index(const Roo::sptr_val& state)
    {
      auto hovered = map_value(state, "hovered-run-index");
      if (!has_value(hovered) || hovered->type != Roo::Value::Type::NUMBER)
        return std::nullopt;
      return hovered->num().get_int();
    }

    Roo::sptr_val set_hovered_run_index(const Roo::sptr_val& state,
                                        const std::optional<HitRun>& hit)
    {
      auto next = state && state->type == Roo::Value::Type::MAP
                    ? Roo::Dict::shallow_copy(state)
                    : Roo::map({});
      Roo::Dict::set_property(
        next,
        Roo::keyword("hovered-run-index"),
        hit ? Roo::number(static_cast<int>(hit->index)) : Roo::Constant::NIL);
      return next;
    }

    std::optional<HitRun> hit_rich_run_for_view(Roo::Context& ctx,
                                                RenderContext& rc,
                                                Runtime::View& view,
                                                const Roo::sptr_val& state,
                                                const Point& pos)
    {
      auto text_op = TextStyle::make_render_op(rc,
                                               view.effective_style,
                                               TextStyle::color(view.effective_style));
      if (!text_op) return std::nullopt;

      const Rect content_rect = view.effective_style.content_rect(view.bounds);
      auto layout = layout_rich_text(rc,
                                     *text_op,
                                     view.effective_style,
                                     rich_runs_from_state(ctx, view.effective_theme, state),
                                     TextStyle::wrap_mode(view.effective_style),
                                     content_rect.w);
      auto hit = hit_rich_run(
        layout,
        TextStyle::alignment(view.effective_style),
        content_rect.w,
        pos.minus(content_rect.x - view.bounds.x, content_rect.y - view.bounds.y));
      if (hit)
      {
        const int scale = std::max(1, view.visual_scale);
        hit->bounds = {
          view.visual_bounds.x + ((content_rect.x - view.bounds.x + hit->bounds.x) * scale),
          view.visual_bounds.y + ((content_rect.y - view.bounds.y + hit->bounds.y) * scale),
          hit->bounds.w * scale,
          hit->bounds.h * scale};
      }
      return hit;
    }

    void emit_rich_text_event(Runtime::View& view,
                              const std::string& event_name,
                              const Roo::sptr_val& payload)
    {
      view.emit_event(
        CustomEvent{Roo::keyword(event_name),
                    payload,
                    view.mode ? Roo::symbol(view.mode->name) : Roo::Constant::NIL});
    }

    namespace Function
    {
      FUNC(RichTextContentSize, rich_text_content_size);
      FUNC(RichTextRender, rich_text_render);
      FUNC(RichTextMouseMotion, rich_text_mouse_motion);
      FUNC(RichTextMouseLeave, rich_text_mouse_leave);
      FUNC(RichTextClick, rich_text_click);

      FUNC_IMPL(RichTextContentSize,
                SIG((FN_ARGS((&Roo::Type::ANY), (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&RichTextContentSize::exec_rich_text_content_size))));

      EXEC_BODY(RichTextContentSize, exec_rich_text_content_size)
      {
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[1]);
        if (!hook_ctx.current_view) return Roo::Constant::NIL;

        RenderContext& rc =
          Roo::obj<RenderContext>(*ctx.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
        auto text_op = TextStyle::make_render_op(rc, hook_ctx.current_view->effective_style);
        if (!text_op) return Roo::Constant::NIL;

        auto layout = layout_rich_text(
          rc,
          *text_op,
          hook_ctx.current_view->effective_style,
          rich_runs_from_state(ctx, hook_ctx.current_view->effective_theme, args[0]),
          TextStyle::wrap_mode(hook_ctx.current_view->effective_style),
          hook_ctx.available_width);
        return Script::DimensionAdapter::make_unique(layout.size.w, layout.size.h);
      }

      FUNC_IMPL(RichTextRender,
                SIG((FN_ARGS((&Roo::Type::ANY), (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&RichTextRender::exec_rich_text_render))));

      EXEC_BODY(RichTextRender, exec_rich_text_render)
      {
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[1]);
        if (!hook_ctx.current_view) return Roo::Constant::NIL;

        RenderContext& rc =
          Roo::obj<RenderContext>(*ctx.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
        Runtime::View& view = *hook_ctx.current_view;
        auto text_op = TextStyle::make_render_op(rc,
                                                 view.effective_style,
                                                 TextStyle::color(view.effective_style));
        if (!text_op) return Roo::Constant::NIL;

        const Rect content_rect = view.effective_style.content_rect(view.bounds);
        auto layout =
          layout_rich_text(rc,
                           *text_op,
                           view.effective_style,
                           rich_runs_from_state(ctx, view.effective_theme, args[0]),
                           TextStyle::wrap_mode(view.effective_style),
                           content_rect.w);

        for (std::size_t line_index = 0; line_index < layout.lines.size(); line_index++)
        {
          const auto& line = layout.lines[line_index];
          int x =
            line_start_x(line, TextStyle::alignment(view.effective_style), content_rect.w);
          int y = static_cast<int>(line_index) * layout.line_height;
          for (const auto& token : line.tokens)
          {
            Text::LayoutLine token_line;
            token_line.text = token.text;
            token_line.segments.push_back({token.text, token.marked, token.width});
            token_line.width = token.width;
            auto token_op = token_render_op(rc, view.effective_style, token, true);
            if (token_op)
            {
              Text::render_layout_line(rc, *token_op, token_line, x, y);
            }
            x += token.width;
          }
        }
        return Roo::Constant::NIL;
      }

      FUNC_IMPL(RichTextMouseMotion,
                SIG((FN_ARGS((&Roo::Type::ANY),
                             (&Pixils::Script::HostType::MOUSE_MOTION_EVENT),
                             (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&RichTextMouseMotion::exec_rich_text_mouse_motion))));

      EXEC_BODY(RichTextMouseMotion, exec_rich_text_mouse_motion)
      {
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[2]);
        if (!hook_ctx.current_view) return args[0];

        RenderContext& rc =
          Roo::obj<RenderContext>(*ctx.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
        Runtime::View& view = *hook_ctx.current_view;
        auto hit = hit_rich_run_for_view(ctx,
                                         rc,
                                         view,
                                         args[0],
                                         Roo::obj<MouseEvent>(*args[1]).local_pos);
        auto previous = hovered_run_index(args[0]);
        if (previous ==
            (hit ? std::optional<int>(static_cast<int>(hit->index)) : std::nullopt))
          return args[0];

        emit_rich_text_event(view,
                             hit ? "rich-text/hover" : "rich-text/leave",
                             hit ? hit_payload(view, *hit) : Roo::Constant::NIL);
        return set_hovered_run_index(args[0], hit);
      }

      FUNC_IMPL(RichTextMouseLeave,
                SIG((FN_ARGS((&Roo::Type::ANY),
                             (&Pixils::Script::HostType::MOUSE_MOTION_EVENT),
                             (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&RichTextMouseLeave::exec_rich_text_mouse_leave))));

      EXEC_BODY(RichTextMouseLeave, exec_rich_text_mouse_leave)
      {
        if (!hovered_run_index(args[0])) return args[0];
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[2]);
        if (hook_ctx.current_view)
          emit_rich_text_event(*hook_ctx.current_view,
                               "rich-text/leave",
                               Roo::Constant::NIL);
        return set_hovered_run_index(args[0], std::nullopt);
      }

      FUNC_IMPL(RichTextClick,
                SIG((FN_ARGS((&Roo::Type::ANY),
                             (&Pixils::Script::HostType::MOUSE_EVENT),
                             (&Pixils::Script::HostType::HOOK_CONTEXT)),
                     EXEC_DISPATCH(&RichTextClick::exec_rich_text_click))));

      EXEC_BODY(RichTextClick, exec_rich_text_click)
      {
        HookContext& hook_ctx = Roo::obj<HookContext>(*args[2]);
        if (!hook_ctx.current_view) return args[0];

        RenderContext& rc =
          Roo::obj<RenderContext>(*ctx.lookup(Script::ID__PIXILS__RENDER_CONTEXT));
        auto hit = hit_rich_run_for_view(ctx,
                                         rc,
                                         *hook_ctx.current_view,
                                         args[0],
                                         Roo::obj<MouseEvent>(*args[1]).local_pos);
        if (hit)
          emit_rich_text_event(*hook_ctx.current_view,
                               "rich-text/click",
                               hit_payload(*hook_ctx.current_view, *hit));
        return args[0];
      }
    } // namespace Function

    Runtime::Mode make_rich_text_component_mode()
    {
      Runtime::Mode mode;
      mode.name = "ui/rich-text";
      mode.selector_modes.push_back(mode.name);
      mode.content_size = Function::RichTextContentSize::make();
      mode.render = Function::RichTextRender::make();
      mode.on_mouse_motion = Function::RichTextMouseMotion::make();
      mode.on_mouse_leave = Function::RichTextMouseLeave::make();
      mode.on_click = Function::RichTextClick::make();
      return mode;
    }
  } // namespace

  void register_rich_text_component(Roo::Runtime& runtime)
  {
    auto modes = runtime.lookup(Script::ID__PIXILS__MODES);
    Roo::Dict::set_property(
      modes,
      Roo::symbol("ui/rich-text"),
      Script::ModeAdapter::make_unique(make_rich_text_component_mode()));
  }
} // namespace Pixils::UI::Components
