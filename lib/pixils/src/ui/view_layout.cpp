#include "pixils/ui/view_layout.h"

#include <pixils/benchmark/counters.h>
#include <pixils/binding/pixils_namespace.h>
#include <pixils/binding/ui/style/style_host_type.h>
#include <pixils/binding/ui/style/theme_definition.h>
#include <pixils/hook_context.h>
#include <pixils/runtime/hook_invocation.h>
#include <pixils/runtime/view.h>
#include <pixils/ui/base_theme.h>
#include <pixils/ui/theme.h>

#include <functional>
#include <lisple/context.h>
#include <lisple/exception.h>
#include <lisple/runtime.h>
#include <lisple/runtime/dict.h>
#include <unordered_map>

namespace Pixils::UI
{
  namespace
  {
    enum class Axis
    {
      HORIZONTAL,
      VERTICAL,
    };

    constexpr size_t MAX_PERSISTENT_NATURAL_SIZE_CHILDREN = 256;

    struct NaturalSizeCacheKey
    {
      const Pixils::Runtime::View* view = nullptr;
      bool has_available_width = false;
      int available_width = 0;
      bool has_available_height = false;
      int available_height = 0;

      bool operator==(const NaturalSizeCacheKey& other) const
      {
        return view == other.view && has_available_width == other.has_available_width &&
               available_width == other.available_width &&
               has_available_height == other.has_available_height &&
               available_height == other.available_height;
      }
    };

    struct NaturalSizeCacheKeyHash
    {
      size_t operator()(const NaturalSizeCacheKey& key) const
      {
        size_t seed = std::hash<const Pixils::Runtime::View*>{}(key.view);
        auto combine = [&](size_t value)
        {
          seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };
        combine(std::hash<bool>{}(key.has_available_width));
        combine(std::hash<int>{}(key.available_width));
        combine(std::hash<bool>{}(key.has_available_height));
        combine(std::hash<int>{}(key.available_height));
        return seed;
      }
    };

    using NaturalSizeCache = std::
      unordered_map<NaturalSizeCacheKey, std::optional<Dimension>, NaturalSizeCacheKeyHash>;

    struct LayoutPass
    {
      Lisple::Runtime& runtime;
      const Lisple::sptr_val& hook_ctx;
      NaturalSizeCache natural_size_cache;
    };

    void hash_combine(size_t& seed, size_t value)
    {
      seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    NaturalSizeCacheKey natural_size_cache_key(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      const std::optional<int>& available_width,
      const std::optional<int>& available_height)
    {
      return NaturalSizeCacheKey{.view = view.get(),
                                 .has_available_width = available_width.has_value(),
                                 .available_width = available_width.value_or(0),
                                 .has_available_height = available_height.has_value(),
                                 .available_height = available_height.value_or(0)};
    }

    void append_view_dependency_signature(size_t& seed,
                                          const std::shared_ptr<Pixils::Runtime::View>& view)
    {
      hash_combine(seed, std::hash<const Pixils::Runtime::View*>{}(view.get()));
      if (!view) return;

      PIXILS_BENCHMARK_COUNT(layout_dependency_signature_nodes);
      hash_combine(seed, std::hash<const Pixils::Runtime::Mode*>{}(view->mode));
      hash_combine(seed, std::hash<const Lisple::Value*>{}(view->state.get()));
      hash_combine(seed, std::hash<bool>{}(view->interaction.hovered));
      hash_combine(seed, std::hash<bool>{}(view->interaction.focused));
      hash_combine(seed, std::hash<bool>{}(view->interaction.focus_within));
      hash_combine(seed, std::hash<std::uint64_t>{}(view->style_view.generation()));
      hash_combine(seed, std::hash<size_t>{}(view->children.size()));

      for (const auto& child : view->children)
      {
        append_view_dependency_signature(seed, child);
      }
    }

    size_t natural_size_dependency_signature(
      const std::shared_ptr<Pixils::Runtime::View>& view)
    {
      PIXILS_BENCHMARK_COUNT(layout_dependency_signature_calls);
      PIXILS_BENCHMARK_TIME_BLOCK(layout_dependency_signature_time_ns);
      size_t seed = 0;
      append_view_dependency_signature(seed, view);

      return seed;
    }

    bool natural_size_cache_matches(
      const Pixils::Runtime::View::NaturalContentSizeCache& cache,
      const std::optional<int>& available_width,
      const std::optional<int>& available_height,
      std::uint64_t style_generation,
      size_t subtree_signature)
    {
      return cache.valid && cache.available_width == available_width &&
             cache.available_height == available_height &&
             cache.style_generation == style_generation &&
             cache.subtree_signature == subtree_signature;
    }

    void remember_natural_content_size(Pixils::Runtime::View& view,
                                       const std::optional<int>& available_width,
                                       const std::optional<int>& available_height,
                                       std::uint64_t style_generation,
                                       size_t subtree_signature,
                                       const std::optional<Dimension>& value)
    {
      PIXILS_BENCHMARK_COUNT(layout_natural_size_persistent_cache_stores);
      view.natural_content_size_cache.valid = true;
      view.natural_content_size_cache.available_width = available_width;
      view.natural_content_size_cache.available_height = available_height;
      view.natural_content_size_cache.style_generation = style_generation;
      view.natural_content_size_cache.subtree_signature = subtree_signature;
      view.natural_content_size_cache.value = value;
    }

    bool rect_equals(const Rect& a, const Rect& b)
    {
      return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
    }

    bool layout_cache_matches(const Pixils::Runtime::View::LayoutCache& cache,
                              const Rect& requested_bounds,
                              std::uint64_t style_generation,
                              size_t dependency_signature)
    {
      return cache.valid && rect_equals(cache.requested_bounds, requested_bounds) &&
             cache.style_generation == style_generation &&
             cache.dependency_signature == dependency_signature;
    }

    void remember_layout(Pixils::Runtime::View& view,
                         const Rect& requested_bounds,
                         std::uint64_t style_generation,
                         size_t dependency_signature)
    {
      view.layout_cache.valid = true;
      view.layout_cache.requested_bounds = requested_bounds;
      view.layout_cache.style_generation = style_generation;
      view.layout_cache.dependency_signature = dependency_signature;
    }

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

    int margin_size(const Style& style, Axis axis)
    {
      if (!style.margin) return 0;
      return axis == Axis::HORIZONTAL ? style.margin->l + style.margin->r
                                      : style.margin->t + style.margin->b;
    }

    int padding_size(const Style& style, Axis axis)
    {
      if (!style.padding) return 0;
      return axis == Axis::HORIZONTAL ? style.padding->l + style.padding->r
                                      : style.padding->t + style.padding->b;
    }

    int border_size(const Style& style, Axis axis)
    {
      if (!style.border) return 0;
      return axis == Axis::HORIZONTAL
               ? style.border->left_thickness() + style.border->right_thickness()
               : style.border->top_thickness() + style.border->bottom_thickness();
    }

    int scale_factor(const Style& style)
    {
      return std::max(1, style.scale.value_or(1));
    }

    bool removes_layout(const Style& style)
    {
      return style.visibility && *style.visibility == Style::Visibility::NONE;
    }

    Rect scaled_external_bounds(const Rect& logical_bounds, const Style& style)
    {
      int scale = scale_factor(style);
      return {logical_bounds.x,
              logical_bounds.y,
              logical_bounds.w * scale,
              logical_bounds.h * scale};
    }

    int scaled_outer_size(const Style& style, Axis axis, int logical_outer_size)
    {
      int margin = margin_size(style, axis);
      return margin + std::max(0, logical_outer_size - margin) * scale_factor(style);
    }

    int logical_outer_size_from_scaled(const Style& style, Axis axis, int scaled_outer_size)
    {
      int margin = margin_size(style, axis);
      int scale = scale_factor(style);
      return margin + std::max(0, scaled_outer_size - margin) / scale;
    }

    std::optional<Dimension> parse_dimension_like(const Lisple::sptr_val& value)
    {
      if (!value || value->type == Lisple::Value::Type::NIL) return std::nullopt;

      if (Pixils::Script::HostType::DIMENSION.is_type_of(*value))
      {
        return Lisple::obj<Dimension>(*value);
      }

      if (value->type == Lisple::Value::Type::MAP)
      {
        auto wv = Lisple::Dict::get_property(value, Lisple::keyword("w"));
        auto hv = Lisple::Dict::get_property(value, Lisple::keyword("h"));
        return Dimension{wv ? wv->num().get_int() : 0, hv ? hv->num().get_int() : 0};
      }

      return std::nullopt;
    }

    bool has_content_size_hook(const std::shared_ptr<Pixils::Runtime::View>& view)
    {
      return view && view->mode && view->mode->content_size &&
             view->mode->content_size->type != Lisple::Value::Type::NIL;
    }

    std::optional<Dimension> invoke_content_size_hook(
      const std::shared_ptr<Pixils::Runtime::View>& child,
      Lisple::Runtime& runtime,
      const Lisple::sptr_val& hook_ctx,
      const std::optional<int>& available_width,
      const std::optional<int>& available_height)
    {
      if (!hook_ctx || hook_ctx->type == Lisple::Value::Type::NIL) return std::nullopt;
      if (!has_content_size_hook(child)) return std::nullopt;

      PIXILS_BENCHMARK_COUNT(layout_content_size_hook_calls);
      PIXILS_BENCHMARK_TIME_BLOCK(layout_content_size_hook_time_ns);

      HookContext& native_hook_ctx = Lisple::obj<HookContext>(*hook_ctx);
      auto previous_width = native_hook_ctx.available_width;
      auto previous_height = native_hook_ctx.available_height;
      native_hook_ctx.available_width = available_width;
      native_hook_ctx.available_height = available_height;

      Lisple::sptr_val_v args = {child->state, hook_ctx};
      auto result =
        Pixils::Runtime::invoke_hook(runtime, child, child->mode->content_size, args);
      native_hook_ctx.available_width = previous_width;
      native_hook_ctx.available_height = previous_height;
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

    std::optional<int> resolve_available_content_size(const Style& style,
                                                      const std::optional<int>& parent_size,
                                                      Axis axis)
    {
      const auto& size = axis_size(style, axis);
      const int margin = margin_size(style, axis);
      const int padding = padding_size(style, axis);
      const int border = border_size(style, axis);

      if (size && size->is_fixed())
      {
        const int bounds_size = fixed_outer_size(style, axis) - margin;
        return std::max(0, bounds_size - padding - border);
      }

      if (parent_size)
      {
        const int bounds_size = *parent_size - margin;
        return std::max(0, bounds_size - padding - border);
      }

      return std::nullopt;
    }

    ThemeMatchContext make_theme_match_context(
      const std::shared_ptr<Pixils::Runtime::View>& view)
    {
      if (!view)
      {
        return {};
      }

      ThemeMatchContext ctx;
      ctx.state = view->state;
      ctx.interaction = view->interaction;
      if (view->mode)
      {
        ctx.mode_names = view->mode->selector_modes;
        ctx.class_names = view->mode->class_names;
      }

      return ctx;
    }

    std::vector<ThemeMatchContext> append_theme_match_context(
      const std::vector<ThemeMatchContext>& path,
      const std::shared_ptr<Pixils::Runtime::View>& view)
    {
      auto child_path = path;
      child_path.push_back(make_theme_match_context(view));
      return child_path;
    }

    std::optional<Style> resolve_style_layer(const Runtime::StyleLayer& layer,
                                             const Theme& theme,
                                             Lisple::Runtime& runtime)
    {
      PIXILS_BENCHMARK_COUNT(style_layer_resolve_calls);

      if (layer.style) return layer.style;
      if (!layer.source || layer.source->type == Lisple::Value::Type::NIL)
        return std::nullopt;

      auto resolved_value =
        Script::resolve_theme_vars(theme, theme.selected_variant, layer.source);
      if (!resolved_value) return std::nullopt;

      Lisple::Context ctx(runtime);
      auto coercion = Script::HostType::STYLE.coerce(ctx, resolved_value);
      if (!coercion.success)
      {
        throw Lisple::TypeError("Invalid inline style after resolving theme vars: " +
                                resolved_value->to_string());
      }

      return Lisple::obj<Style>(*coercion.result);
    }

    std::optional<Style> resolve_style_source(const Lisple::sptr_val& source,
                                              const Theme& theme,
                                              Lisple::Runtime& runtime)
    {
      if (!source || source->type == Lisple::Value::Type::NIL) return std::nullopt;

      PIXILS_BENCHMARK_COUNT(runtime_style_source_resolve_calls);

      auto resolved_value =
        Script::resolve_theme_vars(theme, theme.selected_variant, source);
      if (!resolved_value) return std::nullopt;

      Lisple::Context ctx(runtime);
      auto coercion = Script::HostType::STYLE.coerce(ctx, resolved_value);
      if (!coercion.success)
      {
        throw Lisple::TypeError("Invalid inline style after resolving theme vars: " +
                                resolved_value->to_string());
      }

      return Lisple::obj<Style>(*coercion.result);
    }

    Style resolve_effective_style(const std::shared_ptr<Pixils::Runtime::View>& view,
                                  Lisple::Runtime& runtime,
                                  const Style* inherited_style,
                                  const std::vector<ThemeMatchContext>& selector_path)
    {
      std::optional<Style> resolved_style = std::nullopt;

      if (view->mode)
      {
        for (const Style* theme_style :
             view->effective_theme.get_matching_styles(selector_path))
        {
          if (!resolved_style) resolved_style = Style{};
          apply_style_variant(*resolved_style, *theme_style);
        }

        if (!view->mode->style_layers.empty())
        {
          for (const auto& layer : view->mode->style_layers)
          {
            auto layer_style = resolve_style_layer(layer, view->effective_theme, runtime);
            if (!layer_style) continue;
            if (!resolved_style) resolved_style = Style{};
            apply_style_variant(*resolved_style, *layer_style);
          }
        }

        if (view->mode->style)
        {
          if (!resolved_style) resolved_style = Style{};
          apply_style_variant(*resolved_style, *view->mode->style);
        }

        if (auto runtime_style = resolve_style_source(view->mode->runtime_style_source,
                                                      view->effective_theme,
                                                      runtime))
        {
          if (!resolved_style) resolved_style = Style{};
          apply_style_variant(*resolved_style, *runtime_style);
        }

        if (view->mode->runtime_style)
        {
          if (!resolved_style) resolved_style = Style{};
          apply_style_variant(*resolved_style, *view->mode->runtime_style);
        }
      }

      return resolve_style(
        resolved_style,
        inherited_style,
        view->state,
        view->interaction,
        view->effective_theme.defaults ? &*view->effective_theme.defaults : nullptr);
    }

    std::optional<Theme> lookup_theme(Lisple::Runtime& runtime, const std::string& name)
    {
      auto themes = runtime.lookup(Pixils::Script::ID__PIXILS__THEMES);
      auto theme_val = Lisple::Dict::get_property(themes, Lisple::symbol(name));
      if (!theme_val || theme_val->type == Lisple::Value::Type::NIL) return std::nullopt;
      return Lisple::obj<Theme>(*theme_val);
    }

    Theme resolve_effective_theme_impl(const std::shared_ptr<Pixils::Runtime::View>& view,
                                       Lisple::Runtime& runtime,
                                       const Theme* inherited_theme)
    {
      std::optional<std::string> selected_variant =
        view && view->mode ? view->mode->theme_variant : std::nullopt;
      if (!selected_variant && inherited_theme)
      {
        selected_variant = inherited_theme->selected_variant;
      }
      else if (!selected_variant && view && view->inherited_theme)
      {
        selected_variant = view->inherited_theme->selected_variant;
      }

      Theme theme =
        inherited_theme
          ? inherited_theme->resolved_for_variant(selected_variant)
          : (view && view->inherited_theme
               ? view->inherited_theme->resolved_for_variant(selected_variant)
               : default_base_theme(runtime).resolved_for_variant(selected_variant));
      if (!view || !view->mode || !view->mode->theme) return theme;

      for (const auto& theme_name : *view->mode->theme)
      {
        auto local_theme = lookup_theme(runtime, theme_name);
        if (local_theme)
        {
          overlay_theme(theme, local_theme->resolved_for_variant(selected_variant));
        }
      }

      theme.selected_variant = selected_variant;
      return theme;
    }

    void resolve_style_view_snapshot(const std::shared_ptr<Pixils::Runtime::View>& view,
                                     Lisple::Runtime& runtime,
                                     const Style* inherited_style,
                                     const Theme* inherited_theme,
                                     const std::vector<ThemeMatchContext>& selector_path)
    {
      const auto parent_generation =
        view->style_view.parent() ? view->style_view.parent()->generation() : 0;
      if (view->style_view.valid_for(view->mode,
                                     view->state.get(),
                                     view->interaction,
                                     inherited_theme,
                                     parent_generation))
      {
        PIXILS_BENCHMARK_COUNT(style_view_cache_hits);
        return;
      }

      PIXILS_BENCHMARK_COUNT(style_view_cache_misses);
      view->effective_theme = resolve_effective_theme_impl(view, runtime, inherited_theme);
      view->effective_style =
        resolve_effective_style(view, runtime, inherited_style, selector_path);
      view->style_view.mark_resolved(view->mode,
                                     view->state.get(),
                                     view->interaction,
                                     inherited_theme,
                                     parent_generation);
    }

    std::optional<Dimension> calculate_child_tree_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      LayoutPass& pass,
      const std::optional<int>& available_width,
      const std::optional<int>& available_height,
      const std::vector<ThemeMatchContext>& selector_path);

    std::vector<Rect> layout_children_with_selector_path(
      const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
      const Rect& parent,
      LayoutPass& pass,
      const Style::Layout& layout,
      const Style* inherited_style,
      const Theme* inherited_theme,
      const std::vector<ThemeMatchContext>& parent_selector_path);

    /**
     * Computes the natural content size of a view, resolving its effective theme and
     * style on the first visit. Results are cached in LayoutPass for the duration of
     * the layout pass, so each view is resolved and sized at most once per frame.
     */
    std::optional<Dimension> calculate_natural_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      LayoutPass& pass,
      const std::optional<int>& parent_available_width,
      const std::optional<int>& parent_available_height,
      const Style* inherited_style,
      const Theme* inherited_theme,
      const std::vector<ThemeMatchContext>& selector_path)
    {
      if (!view || !view->mode) return std::nullopt;

      auto cache_key =
        natural_size_cache_key(view, parent_available_width, parent_available_height);
      auto cached = pass.natural_size_cache.find(cache_key);
      if (cached != pass.natural_size_cache.end())
      {
        PIXILS_BENCHMARK_COUNT(layout_natural_size_cache_hits);
        PIXILS_BENCHMARK_COUNT(layout_natural_size_pass_cache_hits);
        return cached->second;
      }

      resolve_style_view_snapshot(view,
                                  pass.runtime,
                                  inherited_style,
                                  inherited_theme,
                                  selector_path);

      const bool cacheable_natural_size =
        has_content_size_hook(view) ||
        (!view->children.empty() &&
         view->children.size() <= MAX_PERSISTENT_NATURAL_SIZE_CHILDREN) ||
        removes_layout(view->effective_style);

      std::uint64_t style_generation = 0;
      size_t subtree_signature = 0;
      if (cacheable_natural_size)
      {
        style_generation = view->style_view.generation();
        subtree_signature = natural_size_dependency_signature(view);
        if (natural_size_cache_matches(view->natural_content_size_cache,
                                       parent_available_width,
                                       parent_available_height,
                                       style_generation,
                                       subtree_signature))
        {
          PIXILS_BENCHMARK_COUNT(layout_natural_size_cache_hits);
          PIXILS_BENCHMARK_COUNT(layout_natural_size_persistent_cache_hits);
          pass.natural_size_cache.emplace(cache_key, view->natural_content_size_cache.value);
          return view->natural_content_size_cache.value;
        }
      }

      PIXILS_BENCHMARK_COUNT(layout_natural_size_cache_misses);

      if (removes_layout(view->effective_style))
      {
        pass.natural_size_cache.emplace(cache_key, std::nullopt);
        remember_natural_content_size(*view,
                                      parent_available_width,
                                      parent_available_height,
                                      style_generation,
                                      subtree_signature,
                                      std::nullopt);
        return std::nullopt;
      }

      auto available_width = resolve_available_content_size(view->effective_style,
                                                            parent_available_width,
                                                            Axis::HORIZONTAL);
      auto available_height = resolve_available_content_size(view->effective_style,
                                                             parent_available_height,
                                                             Axis::VERTICAL);

      if (auto natural = invoke_content_size_hook(view,
                                                  pass.runtime,
                                                  pass.hook_ctx,
                                                  available_width,
                                                  available_height))
      {
        pass.natural_size_cache.emplace(cache_key, natural);
        remember_natural_content_size(*view,
                                      parent_available_width,
                                      parent_available_height,
                                      style_generation,
                                      subtree_signature,
                                      natural);
        return natural;
      }
      if (view->children.empty())
      {
        pass.natural_size_cache.emplace(cache_key, std::nullopt);
        return std::nullopt;
      }

      auto natural = calculate_child_tree_content_size(view,
                                                       pass,
                                                       available_width,
                                                       available_height,
                                                       selector_path);
      pass.natural_size_cache.emplace(cache_key, natural);
      if (cacheable_natural_size)
      {
        const auto resolved_subtree_signature = natural_size_dependency_signature(view);
        remember_natural_content_size(*view,
                                      parent_available_width,
                                      parent_available_height,
                                      style_generation,
                                      resolved_subtree_signature,
                                      natural);
      }
      return natural;
    }

    std::optional<Dimension> calculate_child_tree_content_size(
      const std::shared_ptr<Pixils::Runtime::View>& view,
      LayoutPass& pass,
      const std::optional<int>& available_width,
      const std::optional<int>& available_height,
      const std::vector<ThemeMatchContext>& selector_path)
    {
      const Style& style = view->effective_style;
      LayoutDirection direction = style.layout && style.layout->direction
                                    ? *style.layout->direction
                                    : LayoutDirection::COLUMN;
      bool row = direction == LayoutDirection::ROW;

      int total_main = 0;
      int max_cross = 0;
      int flow_count = 0;

      for (const auto& child : view->children)
      {
        auto child_selector_path = append_theme_match_context(selector_path, child);
        auto child_natural_content_size =
          calculate_natural_content_size(child,
                                         pass,
                                         available_width,
                                         available_height,
                                         &style,
                                         &view->effective_theme,
                                         child_selector_path);

        const Style& child_style = child->effective_style;
        if (removes_layout(child_style)) continue;
        if (child_style.position && *child_style.position == PositionMode::ABSOLUTE)
          continue;
        flow_count++;

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
          total_main += scaled_outer_size(child_style, Axis::HORIZONTAL, child_outer_size.w);
          max_cross =
            std::max(max_cross,
                     scaled_outer_size(child_style, Axis::VERTICAL, child_outer_size.h));
        }
        else
        {
          total_main += scaled_outer_size(child_style, Axis::VERTICAL, child_outer_size.h);
          max_cross =
            std::max(max_cross,
                     scaled_outer_size(child_style, Axis::HORIZONTAL, child_outer_size.w));
        }
      }

      if (style.layout && style.layout->gap && style.layout->gap->mode &&
          *style.layout->gap->mode == Style::Layout::GapMode::FIXED && flow_count > 1)
      {
        total_main += style.layout->gap->size.value_or(0) * (flow_count - 1);
      }

      return row ? Dimension{total_main, max_cross} : Dimension{max_cross, total_main};
    }

    void layout_view_tree_impl(const std::shared_ptr<Pixils::Runtime::View>& view,
                               const Rect& bounds,
                               LayoutPass& pass,
                               const Style* inherited_style,
                               const Theme* inherited_theme,
                               const std::vector<ThemeMatchContext>& selector_path)
    {
      if (!view) return;

      PIXILS_BENCHMARK_COUNT(layout_view_tree_nodes);

      resolve_style_view_snapshot(view,
                                  pass.runtime,
                                  inherited_style,
                                  inherited_theme,
                                  selector_path);
      const auto style_generation = view->style_view.generation();
      const auto dependency_signature = natural_size_dependency_signature(view);
      if (layout_cache_matches(view->layout_cache,
                               bounds,
                               style_generation,
                               dependency_signature))
      {
        PIXILS_BENCHMARK_COUNT(layout_dirty_cache_hits);
        PIXILS_BENCHMARK_ADD(layout_skipped_clean_subtrees, 1);
        return;
      }
      PIXILS_BENCHMARK_COUNT(layout_dirty_cache_misses);

      auto natural_content_size = calculate_natural_content_size(view,
                                                                 pass,
                                                                 bounds.w,
                                                                 bounds.h,
                                                                 inherited_style,
                                                                 inherited_theme,
                                                                 selector_path);
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
      view->external_bounds = scaled_external_bounds(view->bounds, view->effective_style);

      const Style& style = view->effective_style;
      if (removes_layout(style))
      {
        remember_layout(*view, bounds, style_generation, dependency_signature);
        return;
      }
      if (view->children.empty())
      {
        remember_layout(*view, bounds, style_generation, dependency_signature);
        return;
      }

      Rect content = style.content_rect(view->bounds);
      auto child_rects =
        layout_children_with_selector_path(view->children,
                                           content,
                                           pass,
                                           style.layout.value_or(Style::Layout{}),
                                           &style,
                                           &view->effective_theme,
                                           selector_path);

      for (size_t i = 0; i < view->children.size(); i++)
      {
        auto& child_ptr = view->children[i];
        const Style& child_style = child_ptr->effective_style;
        auto child_selector_path = append_theme_match_context(selector_path, child_ptr);

        Rect child_bounds;
        if (child_style.position && *child_style.position == PositionMode::ABSOLUTE)
        {
          int top = child_style.top.value_or(0);
          int left = child_style.left.value_or(0);
          auto child_natural_content_size =
            calculate_natural_content_size(child_ptr,
                                           pass,
                                           content.w,
                                           content.h,
                                           &style,
                                           &view->effective_theme,
                                           child_selector_path);
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

        layout_view_tree_impl(child_ptr,
                              child_bounds,
                              pass,
                              &style,
                              &view->effective_theme,
                              child_selector_path);
      }

      const auto resolved_dependency_signature = natural_size_dependency_signature(view);
      remember_layout(*view, bounds, style_generation, resolved_dependency_signature);
    }

    std::vector<Rect> layout_children_with_selector_path(
      const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
      const Rect& parent,
      LayoutPass& pass,
      const Style::Layout& layout,
      const Style* inherited_style,
      const Theme* inherited_theme,
      const std::vector<ThemeMatchContext>& parent_selector_path)
    {
      PIXILS_BENCHMARK_COUNT(layout_children_calls);
      PIXILS_BENCHMARK_ADD(layout_children_items,
                           static_cast<std::int64_t>(children.size()));

      LayoutDirection direction = layout.direction.value_or(LayoutDirection::COLUMN);
      bool row = direction == LayoutDirection::ROW;
      std::vector<std::optional<Dimension>> natural_content_sizes;
      std::vector<int> logical_outer_sizes(children.size(), 0);
      std::vector<int> outer_sizes(children.size(), 0);
      natural_content_sizes.reserve(children.size());

      for (const auto& child : children)
      {
        auto child_selector_path = append_theme_match_context(parent_selector_path, child);
        natural_content_sizes.push_back(calculate_natural_content_size(child,
                                                                       pass,
                                                                       parent.w,
                                                                       parent.h,
                                                                       inherited_style,
                                                                       inherited_theme,
                                                                       child_selector_path));
      }

      int total_fixed = 0;
      int fill_count = 0;
      int flow_count = 0;
      std::vector<size_t> shrink_indices;
      for (size_t i = 0; i < children.size(); i++)
      {
        const Style& cs = children[i]->effective_style;
        const auto& natural = natural_content_sizes[i];
        if (removes_layout(cs)) continue;
        if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
        flow_count++;

        Axis main_axis = row ? Axis::HORIZONTAL : Axis::VERTICAL;
        if (fills_axis(cs, main_axis, false))
        {
          fill_count++;
        }
        else if (axis_size(cs, main_axis) && axis_size(cs, main_axis)->is_fixed())
        {
          logical_outer_sizes[i] = row ? cs.total_width() : cs.total_height();
          outer_sizes[i] = scaled_outer_size(cs, main_axis, logical_outer_sizes[i]);
          total_fixed += outer_sizes[i];
        }
        else if (axis_size(cs, main_axis) && axis_size(cs, main_axis)->is_shrink())
        {
          if (natural)
          {
            Dimension outer_size = calculate_outer_size(cs, *natural);
            logical_outer_sizes[i] = row ? outer_size.w : outer_size.h;
            outer_sizes[i] = scaled_outer_size(cs, main_axis, logical_outer_sizes[i]);
          }
          shrink_indices.push_back(i);
          total_fixed += outer_sizes[i];
        }
        else if (natural)
        {
          Dimension outer_size = calculate_outer_size(cs, *natural);
          logical_outer_sizes[i] = row ? outer_size.w : outer_size.h;
          outer_sizes[i] = scaled_outer_size(cs, main_axis, logical_outer_sizes[i]);
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
      int overflow = std::max(0, total_fixed + total_fixed_gap - available);
      while (overflow > 0 && !shrink_indices.empty())
      {
        int per_child = std::max(1,
                                 (overflow + static_cast<int>(shrink_indices.size()) - 1) /
                                   static_cast<int>(shrink_indices.size()));
        std::vector<size_t> still_shrinkable;
        for (size_t index : shrink_indices)
        {
          int reduction = std::min(outer_sizes[index], per_child);
          outer_sizes[index] -= reduction;
          overflow -= reduction;
          if (outer_sizes[index] > 0) still_shrinkable.push_back(index);
          if (overflow <= 0) break;
        }
        shrink_indices = std::move(still_shrinkable);
      }

      total_fixed = 0;
      for (size_t i = 0; i < children.size(); i++)
      {
        const Style& cs = children[i]->effective_style;
        if (removes_layout(cs)) continue;
        if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
        if (!fills_axis(cs, row ? Axis::HORIZONTAL : Axis::VERTICAL, false))
          total_fixed += outer_sizes[i];
      }

      int fill_size =
        fill_count > 0 ? (available - total_fixed - total_fixed_gap) / fill_count : 0;
      for (size_t i = 0; i < children.size(); i++)
      {
        const Style& cs = children[i]->effective_style;
        if (removes_layout(cs)) continue;
        if (cs.position && *cs.position == PositionMode::ABSOLUTE) continue;
        if (outer_sizes[i] == 0 &&
            fills_axis(cs, row ? Axis::HORIZONTAL : Axis::VERTICAL, false))
        {
          outer_sizes[i] = fill_size;
          logical_outer_sizes[i] =
            logical_outer_size_from_scaled(cs,
                                           row ? Axis::HORIZONTAL : Axis::VERTICAL,
                                           fill_size);
        }
      }

      int total_flow_size = 0;
      for (size_t i = 0; i < children.size(); i++)
      {
        const Style& cs = children[i]->effective_style;
        if (removes_layout(cs)) continue;
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

      const Style::Layout::AlignItems align_items =
        layout.align_items.value_or(Style::Layout::AlignItems::START);
      int pos = row ? parent.x : parent.y;
      int flow_index = 0;
      for (size_t i = 0; i < children.size(); i++)
      {
        const Style& cs = children[i]->effective_style;
        const Style::Insets margin = cs.margin.value_or(Style::Insets{});

        if (removes_layout(cs))
        {
          rects.push_back({0, 0, 0, 0});
          continue;
        }

        if (cs.position && *cs.position == PositionMode::ABSOLUTE)
        {
          rects.push_back({0, 0, 0, 0});
          continue;
        }

        int outer_size = outer_sizes[i];
        Axis cross_axis = row ? Axis::VERTICAL : Axis::HORIZONTAL;
        int cross_outer_size = resolve_outer_size(
          cs,
          natural_content_sizes[i],
          cross_axis,
          false,
          logical_outer_size_from_scaled(cs, cross_axis, row ? parent.h : parent.w));
        int cross_scaled_outer_size = scaled_outer_size(cs, cross_axis, cross_outer_size);
        int cross_available = row ? parent.h : parent.w;
        int cross_offset = 0;
        switch (align_items)
        {
        case Style::Layout::AlignItems::CENTER:
          cross_offset = std::max(0, (cross_available - cross_scaled_outer_size) / 2);
          break;
        case Style::Layout::AlignItems::END:
          cross_offset = std::max(0, cross_available - cross_scaled_outer_size);
          break;
        default:
          break;
        }

        int logical_outer_size = logical_outer_sizes[i];
        if (logical_outer_size == 0)
        {
          logical_outer_size =
            logical_outer_size_from_scaled(cs,
                                           row ? Axis::HORIZONTAL : Axis::VERTICAL,
                                           outer_size);
        }
        else if (outer_size != scaled_outer_size(cs,
                                                 row ? Axis::HORIZONTAL : Axis::VERTICAL,
                                                 logical_outer_size))
        {
          logical_outer_size =
            logical_outer_size_from_scaled(cs,
                                           row ? Axis::HORIZONTAL : Axis::VERTICAL,
                                           outer_size);
        }

        if (row)
        {
          rects.push_back({pos + margin.l,
                           parent.y + cross_offset + margin.t,
                           std::max(0, logical_outer_size - margin.l - margin.r),
                           std::max(0, cross_outer_size - margin.t - margin.b)});
        }
        else
        {
          rects.push_back({parent.x + cross_offset + margin.l,
                           pos + margin.t,
                           std::max(0, cross_outer_size - margin.l - margin.r),
                           std::max(0, logical_outer_size - margin.t - margin.b)});
        }

        pos += outer_size;
        flow_index++;
        if (flow_index < flow_count) pos += gap_size;
      }

      return rects;
    }
  } // namespace

  Theme resolve_effective_theme(const std::shared_ptr<Pixils::Runtime::View>& view,
                                Lisple::Runtime& runtime,
                                const Theme* inherited_theme)
  {
    return resolve_effective_theme_impl(view, runtime, inherited_theme);
  }

  std::vector<Rect> layout_children(
    const std::vector<std::shared_ptr<Pixils::Runtime::View>>& children,
    const Rect& parent,
    Lisple::Runtime& runtime,
    const Lisple::sptr_val& hook_ctx,
    const Style::Layout& layout,
    const Style* inherited_style,
    const Theme* inherited_theme)
  {
    LayoutPass pass{.runtime = runtime, .hook_ctx = hook_ctx, .natural_size_cache = {}};
    return layout_children_with_selector_path(children,
                                              parent,
                                              pass,
                                              layout,
                                              inherited_style,
                                              inherited_theme,
                                              {});
  }

  void layout_view_tree(const std::shared_ptr<Pixils::Runtime::View>& view,
                        const Rect& bounds,
                        Lisple::Runtime& runtime,
                        const Lisple::sptr_val& hook_ctx)
  {
    PIXILS_BENCHMARK_COUNT(layout_view_tree_calls);
    PIXILS_BENCHMARK_TIME_BLOCK(layout_time_ns);

    LayoutPass pass{.runtime = runtime, .hook_ctx = hook_ctx, .natural_size_cache = {}};
    layout_view_tree_impl(view,
                          bounds,
                          pass,
                          nullptr,
                          nullptr,
                          append_theme_match_context({}, view));
  }

} // namespace Pixils::UI
