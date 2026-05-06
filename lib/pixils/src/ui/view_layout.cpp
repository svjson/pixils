#include "pixils/ui/view_layout.h"

#include <pixils/binding/pixils_namespace.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/view.h>

#include <lisple/runtime/dict.h>

namespace Pixils::UI
{
  namespace
  {
    enum class Axis
    {
      HORIZONTAL,
      VERTICAL,
    };

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

    const std::optional<Style::Size>& axis_size(const Style& style, Axis axis)
    {
      return axis == Axis::HORIZONTAL ? style.width : style.height;
    }

    int fixed_outer_size(const Style& style, Axis axis)
    {
      return axis == Axis::HORIZONTAL ? style.total_width() : style.total_height();
    }

    int natural_outer_size(const Style& style,
                           const std::optional<Dimension>& natural_content_size,
                           Axis axis)
    {
      if (!natural_content_size) return 0;
      Dimension outer = calculate_outer_size(style, *natural_content_size);
      return axis == Axis::HORIZONTAL ? outer.w : outer.h;
    }

    bool fills_axis(const Style& style, Axis axis, bool root_context)
    {
      const auto& size = axis_size(style, axis);
      if (!size) return root_context;
      if (size->is_fill()) return true;
      if (size->is_auto()) return root_context;
      return false;
    }

    int resolve_outer_size(const Style& style,
                           const std::optional<Dimension>& natural_content_size,
                           Axis axis,
                           bool root_context,
                           int available_size)
    {
      const auto& size = axis_size(style, axis);
      if (size && size->is_fixed()) return fixed_outer_size(style, axis);
      if (fills_axis(style, axis, root_context)) return available_size;

      int natural = natural_outer_size(style, natural_content_size, axis);
      if (natural > 0) return natural;

      return 0;
    }

    Style resolve_effective_style(const std::shared_ptr<Pixils::Runtime::View>& view,
                                  const Style* inherited_style)
    {
      return resolve_style(view->mode->style,
                           inherited_style,
                           view->state,
                           view->interaction);
    }

    std::optional<Dimension> calculate_child_tree_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      Lisple::Runtime& runtime,
      const Lisple::sptr_rtval& hook_ctx,
      const Style* inherited_style);

    std::optional<Dimension> calculate_natural_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      Lisple::Runtime& runtime,
      const Lisple::sptr_rtval& hook_ctx,
      const Style* inherited_style)
    {
      if (!view || !view->mode) return std::nullopt;

      view->effective_style = resolve_effective_style(view, inherited_style);

      if (auto natural = invoke_content_size_hook(view, runtime, hook_ctx)) return natural;
      if (view->children.empty()) return std::nullopt;

      return calculate_child_tree_content_size(view,
                                               runtime,
                                               hook_ctx,
                                               &view->effective_style);
    }

    std::optional<Dimension> calculate_child_tree_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      Lisple::Runtime& runtime,
      const Lisple::sptr_rtval& hook_ctx,
      const Style* inherited_style)
    {
      view->effective_style = resolve_effective_style(view, inherited_style);
      const Style& style = view->effective_style;
      LayoutDirection direction = style.layout && style.layout->direction
                                    ? *style.layout->direction
                                    : LayoutDirection::COLUMN;
      bool row = direction == LayoutDirection::ROW;

      int total_main = 0;
      int max_cross = 0;

      for (const auto& child : view->children)
      {
        child->effective_style = resolve_effective_style(child, &style);
        const Style& child_style = child->effective_style;
        if (child_style.position && *child_style.position == PositionMode::ABSOLUTE)
          continue;

        auto child_natural_content_size =
          calculate_natural_content_size(child, runtime, hook_ctx, &style);
        Dimension child_outer_size{0, 0};

        if (child_natural_content_size)
        {
          child_outer_size = calculate_outer_size(child_style, *child_natural_content_size);
        }

        if (child_style.width && child_style.width->is_fixed())
          child_outer_size.w = child_style.total_width();
        if (child_style.height && child_style.height->is_fixed())
          child_outer_size.h = child_style.total_height();

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

    void layout_view_tree_impl(const std::shared_ptr<Pixils::Runtime::View>& view,
                               const Rect& bounds,
                               Lisple::Runtime& runtime,
                               const Lisple::sptr_rtval& hook_ctx,
                               const Style* inherited_style)
    {
      if (!view) return;

      view->effective_style = resolve_effective_style(view, inherited_style);

      auto natural_content_size =
        calculate_natural_content_size(view, runtime, hook_ctx, inherited_style);
      int resolved_w = inherited_style ? bounds.w
                                       : resolve_outer_size(view->effective_style,
                                                            natural_content_size,
                                                            Axis::HORIZONTAL,
                                                            true,
                                                            bounds.w);
      int resolved_h = inherited_style ? bounds.h
                                       : resolve_outer_size(view->effective_style,
                                                            natural_content_size,
                                                            Axis::VERTICAL,
                                                            true,
                                                            bounds.h);
      int resolved_x = bounds.x;
      int resolved_y = bounds.y;
      if (!inherited_style && view->effective_style.position &&
          *view->effective_style.position == PositionMode::ABSOLUTE)
      {
        resolved_x += view->effective_style.left.value_or(0);
        resolved_y += view->effective_style.top.value_or(0);
      }
      view->bounds = {resolved_x, resolved_y, resolved_w, resolved_h};

      const Style& style = view->effective_style;
      if (style.hidden && *style.hidden) return;
      if (view->children.empty()) return;

      Rect content = style.content_rect(view->bounds);
      auto child_rects = layout_children(view->children,
                                         content,
                                         runtime,
                                         hook_ctx,
                                         style.layout.value_or(Style::Layout{}),
                                         &style);

      for (size_t i = 0; i < view->children.size(); i++)
      {
        auto& child_ptr = view->children[i];
        const Style& child_style = child_ptr->effective_style;

        Rect child_bounds;
        if (child_style.position && *child_style.position == PositionMode::ABSOLUTE)
        {
          int top = child_style.top.value_or(0);
          int left = child_style.left.value_or(0);
          auto child_natural_content_size =
            calculate_natural_content_size(child_ptr, runtime, hook_ctx, &style);
          int w = resolve_outer_size(child_style,
                                     child_natural_content_size,
                                     Axis::HORIZONTAL,
                                     false,
                                     content.w);
          int h = resolve_outer_size(child_style,
                                     child_natural_content_size,
                                     Axis::VERTICAL,
                                     false,
                                     content.h);
          child_bounds = {content.x + left, content.y + top, w, h};
        }
        else
        {
          child_bounds = child_rects[i];
        }

        layout_view_tree_impl(child_ptr, child_bounds, runtime, hook_ctx, &style);
      }
    }
  } // namespace

  std::vector<Rect> layout_children(
    const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
    const Rect& parent,
    Lisple::Runtime& runtime,
    const Lisple::sptr_rtval& hook_ctx,
    const Style::Layout& layout,
    const Style* inherited_style)
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
      child->effective_style = resolve_effective_style(child, inherited_style);
      styles.push_back(child->effective_style);
      natural_content_sizes.push_back(
        calculate_natural_content_size(child, runtime, hook_ctx, inherited_style));
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

      Axis main_axis = row ? Axis::HORIZONTAL : Axis::VERTICAL;
      if (fills_axis(cs, main_axis, false))
      {
        fill_count++;
      }
      else if (axis_size(cs, main_axis) && axis_size(cs, main_axis)->is_fixed())
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
    }

    int available = row ? parent.w : parent.h;
    int fixed_gap_size = 0;
    if (layout.gap && layout.gap->mode && flow_count > 1)
    {
      switch (*layout.gap->mode)
      {
      case Style::Layout::GapMode::NONE:
        fixed_gap_size = 0;
        break;
      case Style::Layout::GapMode::FIXED:
        fixed_gap_size = layout.gap->size.value_or(0);
        break;
      case Style::Layout::GapMode::SPACE_BETWEEN:
        fixed_gap_size = 0;
        break;
      }
    }

    int total_fixed_gap = flow_count > 1 ? fixed_gap_size * (flow_count - 1) : 0;
    int fill_size =
      fill_count > 0 ? (available - total_fixed - total_fixed_gap) / fill_count : 0;
    for (size_t i = 0; i < children.size(); i++)
    {
      const Style& cs = styles[i];
      if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
      if (outer_sizes[i] == 0 &&
          fills_axis(cs, row ? Axis::HORIZONTAL : Axis::VERTICAL, false))
        outer_sizes[i] = fill_size;
    }

    int total_flow_size = 0;
    for (size_t i = 0; i < children.size(); i++)
    {
      const Style& cs = styles[i];
      if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
      total_flow_size += outer_sizes[i];
    }

    int gap_size = 0;
    if (layout.gap && layout.gap->mode && flow_count > 1)
    {
      switch (*layout.gap->mode)
      {
      case Style::Layout::GapMode::NONE:
        gap_size = 0;
        break;
      case Style::Layout::GapMode::FIXED:
        gap_size = layout.gap->size.value_or(0);
        break;
      case Style::Layout::GapMode::SPACE_BETWEEN:
        gap_size = std::max(0, available - total_flow_size) / (flow_count - 1);
        break;
      }
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
      int cross_outer_size = resolve_outer_size(cs,
                                                natural_content_sizes[i],
                                                row ? Axis::VERTICAL : Axis::HORIZONTAL,
                                                false,
                                                row ? parent.h : parent.w);

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
    layout_view_tree_impl(view, bounds, runtime, hook_ctx, nullptr);
  }

} // namespace Pixils::UI
