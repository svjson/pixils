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
      {MetricId::layout_content_size_hook_time_ns, "layout_content_size_hook_time_ns"},
      {MetricId::layout_natural_size_cache_hits, "layout_natural_size_cache_hits"},
      {MetricId::layout_natural_size_pass_cache_hits, "layout_natural_size_pass_cache_hits"},
      {MetricId::layout_natural_size_persistent_cache_hits,
       "layout_natural_size_persistent_cache_hits"},
      {MetricId::layout_natural_size_persistent_cache_stores,
       "layout_natural_size_persistent_cache_stores"},
      {MetricId::layout_natural_size_cache_misses, "layout_natural_size_cache_misses"},
      {MetricId::layout_dependency_signature_calls, "layout_dependency_signature_calls"},
      {MetricId::layout_dependency_signature_nodes, "layout_dependency_signature_nodes"},
      {MetricId::layout_dependency_signature_time_ns, "layout_dependency_signature_time_ns"},
      {MetricId::layout_dirty_cache_hits, "layout_dirty_cache_hits"},
      {MetricId::layout_dirty_cache_misses, "layout_dirty_cache_misses"},
      {MetricId::layout_invalidations, "layout_invalidations"},
      {MetricId::layout_skipped_clean_subtrees, "layout_skipped_clean_subtrees"},
      {MetricId::layout_dirty_subtree_nodes, "layout_dirty_subtree_nodes"},
      {MetricId::layout_time_ns, "layout_time_ns"},
      {MetricId::render_view_calls, "render_view_calls"},
      {MetricId::render_view_nodes, "render_view_nodes"},
      {MetricId::render_hook_calls, "render_hook_calls"},
      {MetricId::render_hook_time_ns, "render_hook_time_ns"},
      {MetricId::render_temporary_texture_creations, "render_temporary_texture_creations"},
      {MetricId::render_offscreen_passes, "render_offscreen_passes"},
      {MetricId::render_texture_queries, "render_texture_queries"},
      {MetricId::render_copy_calls, "render_copy_calls"},
      {MetricId::render_fill_rect_calls, "render_fill_rect_calls"},
      {MetricId::render_image_lookup_calls, "render_image_lookup_calls"},
      {MetricId::render_time_ns, "render_time_ns"},
      {MetricId::runtime_push_mode_calls, "runtime_push_mode_calls"},
      {MetricId::runtime_pop_mode_calls, "runtime_pop_mode_calls"},
      {MetricId::runtime_render_mode_calls, "runtime_render_mode_calls"},
      {MetricId::runtime_update_mode_calls, "runtime_update_mode_calls"},
      {MetricId::runtime_render_time_ns, "runtime_render_time_ns"},
      {MetricId::runtime_update_time_ns, "runtime_update_time_ns"},
      {MetricId::view_state_equality_checks, "view_state_equality_checks"},
      {MetricId::view_state_assignments_preserved, "view_state_assignments_preserved"},
      {MetricId::view_state_assignments_replaced, "view_state_assignments_replaced"},
      {MetricId::event_handler_invocations, "event_handler_invocations"},
      {MetricId::events_bubbled, "events_bubbled"},
      {MetricId::ui_style_allocations, "ui_style_allocations"},
      {MetricId::ui_style_default_constructed, "ui_style_default_constructed"},
      {MetricId::ui_style_copied, "ui_style_copied"},
      {MetricId::ui_style_assigned, "ui_style_assigned"},
      {MetricId::ui_style_hover_allocations, "ui_style_hover_allocations"},
      {MetricId::ui_style_focus_within_allocations, "ui_style_focus_within_allocations"},
      {MetricId::ui_style_focus_allocations, "ui_style_focus_allocations"},
      {MetricId::style_resolve_calls, "style_resolve_calls"},
      {MetricId::style_resolve_time_ns, "style_resolve_time_ns"},
      {MetricId::style_variant_apply_calls, "style_variant_apply_calls"},
      {MetricId::style_layer_resolve_calls, "style_layer_resolve_calls"},
      {MetricId::runtime_style_source_resolve_calls, "runtime_style_source_resolve_calls"},
      {MetricId::theme_matching_calls, "theme_matching_calls"},
      {MetricId::theme_index_candidate_rules, "theme_index_candidate_rules"},
      {MetricId::theme_full_selector_match_checks, "theme_full_selector_match_checks"},
      {MetricId::theme_rule_match_checks, "theme_rule_match_checks"},
      {MetricId::theme_rules_rejected, "theme_rules_rejected"},
      {MetricId::theme_rules_matched, "theme_rules_matched"},
      {MetricId::style_view_cache_hits, "style_view_cache_hits"},
      {MetricId::style_view_cache_misses, "style_view_cache_misses"},
      {MetricId::style_view_resolutions, "style_view_resolutions"},
      {MetricId::style_view_invalidations, "style_view_invalidations"},
      {MetricId::style_view_property_lookup_calls, "style_view_property_lookup_calls"},
      {MetricId::style_view_property_cache_hits, "style_view_property_cache_hits"},
      {MetricId::style_view_property_cache_misses, "style_view_property_cache_misses"},
      {MetricId::style_view_parent_steps, "style_view_parent_steps"},
      {MetricId::style_view_inherited_fallthroughs, "style_view_inherited_fallthroughs"},
      {MetricId::text_render_op_creations, "text_render_op_creations"},
      {MetricId::text_render_op_failures, "text_render_op_failures"},
      {MetricId::text_measure_calls, "text_measure_calls"},
      {MetricId::text_measure_time_ns, "text_measure_time_ns"},
      {MetricId::text_line_measure_calls, "text_line_measure_calls"},
      {MetricId::text_layout_calls, "text_layout_calls"},
      {MetricId::text_layout_time_ns, "text_layout_time_ns"},
      {MetricId::text_render_calls, "text_render_calls"},
      {MetricId::text_render_time_ns, "text_render_time_ns"},
      {MetricId::text_render_lines, "text_render_lines"},
      {MetricId::text_render_segments, "text_render_segments"},
      {MetricId::text_renderer_size_calls, "text_renderer_size_calls"},
      {MetricId::text_renderer_glyphs_measured, "text_renderer_glyphs_measured"},
      {MetricId::text_renderer_glyphs_rendered, "text_renderer_glyphs_rendered"},
      {MetricId::update_view_tree_calls, "update_view_tree_calls"},
      {MetricId::update_view_tree_nodes, "update_view_tree_nodes"},
      {MetricId::update_hook_calls, "update_hook_calls"},
      {MetricId::update_hook_time_ns, "update_hook_time_ns"},
      {MetricId::update_state_extract_calls, "update_state_extract_calls"},
      {MetricId::update_state_extract_time_ns, "update_state_extract_time_ns"},
      {MetricId::update_state_merge_calls, "update_state_merge_calls"},
      {MetricId::update_state_merge_time_ns, "update_state_merge_time_ns"},
      {MetricId::update_interaction_checks, "update_interaction_checks"},
      {MetricId::update_interaction_changes, "update_interaction_changes"},
      {MetricId::layout_after_layout_hook_calls, "layout_after_layout_hook_calls"},
      {MetricId::layout_after_layout_hook_time_ns, "layout_after_layout_hook_time_ns"},
      {MetricId::layout_after_layout_hook_state_changes,
       "layout_after_layout_hook_state_changes"},
      {MetricId::layout_passes, "layout_passes"},
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
