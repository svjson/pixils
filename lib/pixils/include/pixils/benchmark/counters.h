#ifndef PIXILS__BENCHMARK__COUNTERS_H
#define PIXILS__BENCHMARK__COUNTERS_H

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace Pixils::Benchmark
{
  enum class MetricId : std::uint16_t
  {
    layout_view_tree_calls,
    layout_view_tree_nodes,
    layout_children_calls,
    layout_children_items,
    layout_content_size_hook_calls,
    layout_natural_size_cache_hits,
    render_view_calls,
    render_view_nodes,
    render_hook_calls,
    runtime_push_mode_calls,
    runtime_pop_mode_calls,
    runtime_render_mode_calls,
    runtime_update_mode_calls,
    event_handler_invocations,
    events_bubbled,
    ui_style_allocations,
    ui_style_default_constructed,
    ui_style_copied,
    ui_style_assigned,
    ui_style_hover_allocations,
    ui_style_focus_within_allocations,
    ui_style_focus_allocations,
    style_resolve_calls,
    style_variant_apply_calls,
    style_layer_resolve_calls,
    runtime_style_source_resolve_calls,
    theme_matching_calls,
    theme_rule_match_checks,
    theme_rules_matched,
    count,
  };

  struct MetricDefinition
  {
    MetricId id;
    std::string_view name;
  };

#ifdef PIXILS_ENABLE_BENCHMARK_COUNTERS
  void add(MetricId id, std::int64_t amount = 1);
  void reset();
  std::int64_t value(MetricId id);
  const MetricDefinition* metric_definitions();
  std::size_t metric_definition_count();
#endif
} // namespace Pixils::Benchmark

#ifdef PIXILS_ENABLE_BENCHMARK_COUNTERS
#define PIXILS_BENCHMARK_COUNT(metric_id) \
  ::Pixils::Benchmark::add(::Pixils::Benchmark::MetricId::metric_id)
#define PIXILS_BENCHMARK_ADD(metric_id, amount) \
  ::Pixils::Benchmark::add(::Pixils::Benchmark::MetricId::metric_id, (amount))
#else
#define PIXILS_BENCHMARK_COUNT(metric_id) ((void)0)
#define PIXILS_BENCHMARK_ADD(metric_id, amount) ((void)0)
#endif

#endif /* PIXILS__BENCHMARK__COUNTERS_H */
