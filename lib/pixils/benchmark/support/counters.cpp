#include "benchmark.h"

#include <algorithm>
#include <array>

namespace Pixils::Benchmark
{
  namespace
  {
    std::array<std::int64_t, METRIC_COUNT> counters{};

    constexpr std::array<MetricDefinition, METRIC_COUNT> definitions = {{
      {MetricId::layout_view_tree_calls, "layout_view_tree_calls"},
      {MetricId::layout_view_tree_nodes, "layout_view_tree_nodes"},
      {MetricId::layout_children_calls, "layout_children_calls"},
      {MetricId::layout_children_items, "layout_children_items"},
      {MetricId::layout_content_size_hook_calls, "layout_content_size_hook_calls"},
      {MetricId::layout_natural_size_cache_hits, "layout_natural_size_cache_hits"},
      {MetricId::render_view_calls, "render_view_calls"},
      {MetricId::render_view_nodes, "render_view_nodes"},
      {MetricId::render_hook_calls, "render_hook_calls"},
      {MetricId::runtime_push_mode_calls, "runtime_push_mode_calls"},
      {MetricId::runtime_pop_mode_calls, "runtime_pop_mode_calls"},
      {MetricId::runtime_render_mode_calls, "runtime_render_mode_calls"},
      {MetricId::runtime_update_mode_calls, "runtime_update_mode_calls"},
      {MetricId::event_handler_invocations, "event_handler_invocations"},
      {MetricId::events_bubbled, "events_bubbled"},
      {MetricId::ui_style_allocations, "ui_style_allocations"},
      {MetricId::ui_style_default_constructed, "ui_style_default_constructed"},
      {MetricId::ui_style_copied, "ui_style_copied"},
      {MetricId::ui_style_assigned, "ui_style_assigned"},
      {MetricId::ui_style_hover_allocations, "ui_style_hover_allocations"},
      {MetricId::ui_style_focus_within_allocations,
       "ui_style_focus_within_allocations"},
      {MetricId::ui_style_focus_allocations, "ui_style_focus_allocations"},
      {MetricId::style_resolve_calls, "style_resolve_calls"},
      {MetricId::style_variant_apply_calls, "style_variant_apply_calls"},
      {MetricId::style_layer_resolve_calls, "style_layer_resolve_calls"},
      {MetricId::runtime_style_source_resolve_calls,
       "runtime_style_source_resolve_calls"},
      {MetricId::theme_matching_calls, "theme_matching_calls"},
      {MetricId::theme_rule_match_checks, "theme_rule_match_checks"},
      {MetricId::theme_rules_matched, "theme_rules_matched"},
    }};

    constexpr std::size_t metric_index(MetricId id)
    {
      return static_cast<std::size_t>(id);
    }
  } // namespace

  void add(MetricId id, std::int64_t amount)
  {
    counters[metric_index(id)] += amount;
  }

  void reset()
  {
    std::fill(counters.begin(), counters.end(), 0);
  }

  std::int64_t value(MetricId id)
  {
    return counters[metric_index(id)];
  }

  const MetricDefinition* metric_definitions()
  {
    return definitions.data();
  }

  std::size_t metric_definition_count()
  {
    return definitions.size();
  }

  std::string_view metric_name(MetricId id)
  {
    return definitions[metric_index(id)].name;
  }
} // namespace Pixils::Benchmark
