#include "pixils/ui/view_layout.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/view.h>

#include <lisple/runtime/dict.h>

namespace Pixils::UI
{
  namespace
  {
    Dimension calculate_outer_size(const Style& style, const Dimension& content_size)
    {
      int mar = style.margin ? style.margin->l + style.margin->r : 0;
      int pad = style.padding ? style.padding->l + style.padding->r : 0;
      int bord =
        style.border ? style.border->left_thickness() + style.border->right_thickness() : 0;
      int total_w = content_size.w + mar + pad + bord;

      mar = style.margin ? style.margin->t + style.margin->b : 0;
      pad = style.padding ? style.padding->t + style.padding->b : 0;
      bord =
        style.border ? style.border->top_thickness() + style.border->bottom_thickness() : 0;
      int total_h = content_size.h + mar + pad + bord;

      return Dimension{total_w, total_h};
    }

    std::optional<Dimension> parse_dimension_like(const Lisple::sptr_rtval& value)
    {
      if (!value || value->type == Lisple::RTValue::Type::NIL) return std::nullopt;

      if (Pixils::Script::HostType::DIMENSION.is_type_of(*value))
      {
        return Lisple::obj<Dimension>(*value);
      }

      if (value->type == Lisple::RTValue::Type::MAP)
      {
        auto wv = Lisple::Dict::get_property(value, Lisple::RTValue::keyword("w"));
        auto hv = Lisple::Dict::get_property(value, Lisple::RTValue::keyword("h"));
        return Dimension{wv ? wv->num().get_int() : 0, hv ? hv->num().get_int() : 0};
      }

      return std::nullopt;
    }

    std::optional<Dimension> invoke_content_size_hook(
      const std::shared_ptr<Pixils::Runtime::View>& child,
      Lisple::Runtime& runtime,
      const Lisple::sptr_rtval& hook_ctx)
    {
      if (!hook_ctx || hook_ctx->type == Lisple::RTValue::Type::NIL) return std::nullopt;
      if (!child->mode || !child->mode->content_size ||
          child->mode->content_size->type == Lisple::RTValue::Type::NIL)
        return std::nullopt;

      Lisple::sptr_rtval_v args = {child->state, hook_ctx};
      auto result =
        Pixils::Runtime::invoke_hook(runtime, child, child->mode->content_size, args);
      return parse_dimension_like(result);
    }

    std::optional<Dimension> calculate_child_tree_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      Lisple::Runtime& runtime,
      const Lisple::sptr_rtval& hook_ctx);

    std::optional<Dimension> calculate_natural_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      Lisple::Runtime& runtime,
      const Lisple::sptr_rtval& hook_ctx)
    {
      if (!view || !view->mode) return std::nullopt;

      if (auto measured = invoke_content_size_hook(view, runtime, hook_ctx)) return measured;
      if (view->children.empty()) return std::nullopt;

      return calculate_child_tree_content_size(view, runtime, hook_ctx);
    }

    std::optional<Dimension> calculate_child_tree_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      Lisple::Runtime& runtime,
      const Lisple::sptr_rtval& hook_ctx)
    {
      Style style = resolve_style(view->mode->style, view->state, view->interaction);
      LayoutDirection direction = style.layout && style.layout->direction
                                    ? *style.layout->direction
                                    : LayoutDirection::COLUMN;
      bool row = direction == LayoutDirection::ROW;

      int total_main = 0;
      int max_cross = 0;

      for (const auto& child : view->children)
      {
        Style child_style =
          resolve_style(child->mode->style, child->state, child->interaction);
        if (child_style.position && *child_style.position == PositionMode::ABSOLUTE)
          continue;

        auto child_natural_content_size =
          calculate_natural_content_size(child, runtime, hook_ctx);
        Dimension child_outer_size{0, 0};

        if (child_natural_content_size)
        {
          child_outer_size = calculate_outer_size(child_style, *child_natural_content_size);
        }

        if (child_style.width)
        {
          child_outer_size.w = child_style.total_width();
        }

        if (child_style.height)
        {
          child_outer_size.h = child_style.total_height();
        }

        if (row)
        {
          total_main += child_outer_size.w;
          max_cross = std::max(max_cross, child_outer_size.h);
        }
        else
        {
          total_main += child_outer_size.h;
          max_cross = std::max(max_cross, child_outer_size.w);
        }
      }

      return row ? Dimension{total_main, max_cross} : Dimension{max_cross, total_main};
    }
  } // namespace

  std::vector<Rect> layout_children(
    const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
    const Rect& parent,
    Lisple::Runtime& runtime,
    const Lisple::sptr_rtval& hook_ctx,
    const Style::Layout& layout)
  {
    LayoutDirection direction = layout.direction.value_or(LayoutDirection::COLUMN);
    bool row = direction == LayoutDirection::ROW;
    std::vector<Style> styles;
    std::vector<std::optional<Dimension>> natural_content_sizes;
    std::vector<int> outer_sizes(children.size(), 0);
    styles.reserve(children.size());
    natural_content_sizes.reserve(children.size());

    for (const auto& child : children)
    {
      styles.push_back(resolve_style(child->mode->style, child->state, child->interaction));
      natural_content_sizes.push_back(invoke_content_size_hook(child, runtime, hook_ctx));
    }

    int total_fixed = 0;
    int fill_count = 0;
    int flow_count = 0;
    for (size_t i = 0; i < children.size(); i++)
    {
      const Style& cs = styles[i];
      const auto& natural = natural_content_sizes[i];
      if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
      flow_count++;

      const auto& main_axis_size = row ? cs.width : cs.height;
      if (main_axis_size)
      {
        outer_sizes[i] = row ? cs.total_width() : cs.total_height();
        total_fixed += outer_sizes[i];
      }
      else if (natural)
      {
        Dimension outer_size = calculate_outer_size(cs, *natural);
        outer_sizes[i] = row ? outer_size.w : outer_size.h;
        total_fixed += outer_sizes[i];
      }
      else
      {
        fill_count++;
      }
    }

    int available = row ? parent.w : parent.h;
    int fill_size = fill_count > 0 ? (available - total_fixed) / fill_count : 0;
    for (size_t i = 0; i < children.size(); i++)
    {
      const Style& cs = styles[i];
      if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
      if (outer_sizes[i] == 0) outer_sizes[i] = fill_size;
    }

    int total_flow_size = 0;
    for (size_t i = 0; i < children.size(); i++)
    {
      const Style& cs = styles[i];
      if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
      total_flow_size += outer_sizes[i];
    }

    int gap_size = 0;
    if (layout.gap && layout.gap->mode &&
        *layout.gap->mode == Style::Layout::GapMode::SPACE_BETWEEN && flow_count > 1)
    {
      gap_size = std::max(0, available - total_flow_size) / (flow_count - 1);
    }

    std::vector<Rect> rects;
    rects.reserve(children.size());

    int pos = row ? parent.x : parent.y;
    int flow_index = 0;
    for (size_t i = 0; i < children.size(); i++)
    {
      const Style& cs = styles[i];
      const Style::Insets margin = cs.margin.value_or(Style::Insets{});

      if (cs.position && *cs.position == PositionMode::ABSOLUTE)
      {
        rects.push_back({0, 0, 0, 0});
        continue;
      }

      int outer_size = outer_sizes[i];

      int cross_outer_size = row ? (cs.height ? cs.total_height() : parent.h)
                                 : (cs.width ? cs.total_width() : parent.w);

      if (row)
      {
        rects.push_back({pos + margin.l,
                         parent.y + margin.t,
                         std::max(0, outer_size - margin.l - margin.r),
                         std::max(0, cross_outer_size - margin.t - margin.b)});
      }
      else
      {
        rects.push_back({parent.x + margin.l,
                         pos + margin.t,
                         std::max(0, cross_outer_size - margin.l - margin.r),
                         std::max(0, outer_size - margin.t - margin.b)});
      }

      pos += outer_size;
      flow_index++;
      if (flow_index < flow_count) pos += gap_size;
    }

    return rects;
  }

  void layout_view_tree(const std::shared_ptr<Pixils::Runtime::View>& view,
                        const Rect& bounds,
                        Lisple::Runtime& runtime,
                        const Lisple::sptr_rtval& hook_ctx)
  {
    if (!view) return;

    view->bounds = bounds;

    Style style_res = resolve_style(view->mode->style, view->state, view->interaction);
    if (style_res.hidden && *style_res.hidden) return;
    if (view->children.empty()) return;

    Rect content = style_res.content_rect(bounds);
    auto child_rects = layout_children(view->children,
                                       content,
                                       runtime,
                                       hook_ctx,
                                       style_res.layout.value_or(Style::Layout{}));

    for (size_t i = 0; i < view->children.size(); i++)
    {
      auto& child_ptr = view->children[i];
      Pixils::Runtime::View& child = *child_ptr;
      Style cs = resolve_style(child.mode->style, child.state, child.interaction);

      Rect child_bounds;
      if (cs.position && *cs.position == PositionMode::ABSOLUTE)
      {
        int top = cs.top.value_or(0);
        int left = cs.left.value_or(0);
        int w = cs.width.has_value() ? cs.total_width() : content.w;
        int h = cs.height.has_value() ? cs.total_height() : content.h;
        child_bounds = {content.x + left, content.y + top, w, h};
      }
      else
      {
        child_bounds = child_rects[i];
      }

      layout_view_tree(child_ptr, child_bounds, runtime, hook_ctx);
    }
  }

} // namespace Pixils::UI
