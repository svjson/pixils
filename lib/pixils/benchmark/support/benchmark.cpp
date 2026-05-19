#include "benchmark.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace Pixils::Benchmark
{
  namespace
  {
    Config active_config;
    std::int64_t consumed_value = 0;

    std::string csv_field(std::string_view value)
    {
      bool quote = false;
      for (char ch : value)
      {
        if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r')
        {
          quote = true;
          break;
        }
      }

      if (!quote) return std::string(value);

      std::string escaped = "\"";
      for (char ch : value)
      {
        if (ch == '"') escaped += '"';
        escaped += ch;
      }
      escaped += '"';
      return escaped;
    }

    std::string safe_path_part(std::string value)
    {
      for (char& ch : value)
      {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == ',' || ch == ' ')
        {
          ch = '_';
        }
      }
      return value;
    }

    MetricValues snapshot_metrics()
    {
      MetricValues values{};
      for (std::size_t i = 0; i < METRIC_COUNT; i++)
      {
        values[i] = value(static_cast<MetricId>(i));
      }
      return values;
    }

    std::string csv_header(const Category& category)
    {
      std::ostringstream out;
      out << "timestamp,goalpost,run,category,benchmark,iterations,total_time_ms,"
             "mean_time_ms";
      for (MetricId id : category.metrics)
      {
        out << "," << metric_name(id);
      }
      return out.str();
    }

    std::string status_path(std::string_view line)
    {
      if (line.size() < 4) return {};
      std::string path(line.substr(3));
      const auto rename_arrow = path.find(" -> ");
      if (rename_arrow != std::string::npos)
      {
        path = path.substr(rename_arrow + 4);
      }
      if (!path.empty() && path.front() == '"') return {};
      return path;
    }

    bool path_is_under_benchmark_harness(const std::string& path)
    {
      constexpr std::string_view prefix = "lib/pixils/benchmark/";
      return path.rfind(prefix, 0) == 0;
    }

    std::vector<std::string> dirty_tracked_paths_in_pixils_library()
    {
      std::vector<std::string> dirty;
      std::array<char, 512> buffer{};
      FILE* pipe = popen("git status --porcelain=v1 --untracked-files=no -- lib/pixils "
                         "2>/dev/null",
                         "r");
      if (!pipe) return dirty;

      std::string output;
      while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
      {
        output += buffer.data();
      }
      pclose(pipe);

      std::istringstream lines(output);
      std::string line;
      while (std::getline(lines, line))
      {
        const auto path = status_path(line);
        if (path.empty() || path_is_under_benchmark_harness(path)) continue;
        dirty.push_back(path);
      }
      return dirty;
    }
  } // namespace

  Config& config()
  {
    return active_config;
  }

  Category category(std::string name, std::initializer_list<MetricId> metrics)
  {
    return Category{.name = std::move(name), .metrics = metrics};
  }

  Category layout_category()
  {
    return category("layout",
                    {MetricId::layout_view_tree_calls,
                     MetricId::layout_view_tree_nodes,
                     MetricId::layout_children_calls,
                     MetricId::layout_children_items,
                     MetricId::layout_content_size_hook_calls,
                     MetricId::layout_content_size_hook_time_ns,
                     MetricId::layout_natural_size_cache_hits,
                     MetricId::layout_natural_size_pass_cache_hits,
                     MetricId::layout_natural_size_persistent_cache_hits,
                     MetricId::layout_natural_size_persistent_cache_stores,
                     MetricId::layout_natural_size_cache_misses,
                     MetricId::layout_dependency_signature_calls,
                     MetricId::layout_dependency_signature_nodes,
                     MetricId::layout_dependency_signature_time_ns,
                     MetricId::layout_dirty_cache_hits,
                     MetricId::layout_dirty_cache_misses,
                     MetricId::layout_invalidations,
                     MetricId::layout_skipped_clean_subtrees,
                     MetricId::layout_dirty_subtree_nodes,
                     MetricId::layout_time_ns,
                     MetricId::ui_style_allocations,
                     MetricId::ui_style_default_constructed,
                     MetricId::ui_style_copied,
                     MetricId::ui_style_assigned,
                     MetricId::ui_style_hover_allocations,
                     MetricId::ui_style_focus_within_allocations,
                     MetricId::ui_style_focus_allocations,
                     MetricId::style_resolve_calls,
                     MetricId::style_resolve_time_ns,
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_index_candidate_rules,
                     MetricId::theme_full_selector_match_checks,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_rejected,
                     MetricId::theme_rules_matched,
                     MetricId::style_view_cache_hits,
                     MetricId::style_view_cache_misses,
                     MetricId::style_view_resolutions,
                     MetricId::style_view_invalidations,
                     MetricId::style_view_property_lookup_calls,
                     MetricId::style_view_property_cache_hits,
                     MetricId::style_view_property_cache_misses,
                     MetricId::style_view_parent_steps,
                     MetricId::style_view_inherited_fallthroughs,
                     MetricId::text_render_op_creations,
                     MetricId::text_render_op_failures,
                     MetricId::text_measure_calls,
                     MetricId::text_measure_time_ns,
                     MetricId::text_line_measure_calls,
                     MetricId::text_layout_calls,
                     MetricId::text_layout_time_ns});
  }

  Category render_category()
  {
    return category("render",
                    {MetricId::render_view_calls,
                     MetricId::render_view_nodes,
                     MetricId::render_hook_calls,
                     MetricId::render_hook_time_ns,
                     MetricId::render_temporary_texture_creations,
                     MetricId::render_offscreen_passes,
                     MetricId::render_texture_queries,
                     MetricId::render_copy_calls,
                     MetricId::render_fill_rect_calls,
                     MetricId::render_image_lookup_calls,
                     MetricId::render_time_ns,
                     MetricId::layout_view_tree_calls,
                     MetricId::layout_view_tree_nodes,
                     MetricId::layout_time_ns,
                     MetricId::ui_style_allocations,
                     MetricId::ui_style_default_constructed,
                     MetricId::ui_style_copied,
                     MetricId::ui_style_assigned,
                     MetricId::ui_style_hover_allocations,
                     MetricId::ui_style_focus_within_allocations,
                     MetricId::ui_style_focus_allocations,
                     MetricId::style_resolve_calls,
                     MetricId::style_resolve_time_ns,
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_index_candidate_rules,
                     MetricId::theme_full_selector_match_checks,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_rejected,
                     MetricId::theme_rules_matched,
                     MetricId::style_view_cache_hits,
                     MetricId::style_view_cache_misses,
                     MetricId::style_view_resolutions,
                     MetricId::style_view_invalidations,
                     MetricId::text_render_calls,
                     MetricId::text_render_time_ns,
                     MetricId::text_render_lines,
                     MetricId::text_render_segments,
                     MetricId::text_renderer_size_calls,
                     MetricId::text_renderer_glyphs_measured,
                     MetricId::text_renderer_glyphs_rendered});
  }

  Category runtime_category()
  {
    return category("runtime",
                    {MetricId::runtime_push_mode_calls,
                     MetricId::runtime_pop_mode_calls,
                     MetricId::runtime_render_mode_calls,
                     MetricId::runtime_update_mode_calls,
                     MetricId::runtime_render_time_ns,
                     MetricId::runtime_update_time_ns,
                     MetricId::view_state_equality_checks,
                     MetricId::view_state_assignments_preserved,
                     MetricId::view_state_assignments_replaced,
                     MetricId::event_handler_invocations,
                     MetricId::events_bubbled});
  }

  Category ui_category()
  {
    return category("ui",
                    {MetricId::layout_view_tree_calls,
                     MetricId::layout_view_tree_nodes,
                     MetricId::layout_children_calls,
                     MetricId::layout_children_items,
                     MetricId::layout_content_size_hook_calls,
                     MetricId::layout_content_size_hook_time_ns,
                     MetricId::layout_natural_size_cache_hits,
                     MetricId::layout_natural_size_pass_cache_hits,
                     MetricId::layout_natural_size_persistent_cache_hits,
                     MetricId::layout_natural_size_persistent_cache_stores,
                     MetricId::layout_natural_size_cache_misses,
                     MetricId::layout_dependency_signature_calls,
                     MetricId::layout_dependency_signature_nodes,
                     MetricId::layout_dependency_signature_time_ns,
                     MetricId::layout_dirty_cache_hits,
                     MetricId::layout_dirty_cache_misses,
                     MetricId::layout_invalidations,
                     MetricId::layout_skipped_clean_subtrees,
                     MetricId::layout_dirty_subtree_nodes,
                     MetricId::layout_time_ns,
                     MetricId::render_view_calls,
                     MetricId::render_view_nodes,
                     MetricId::render_hook_calls,
                     MetricId::render_hook_time_ns,
                     MetricId::render_temporary_texture_creations,
                     MetricId::render_offscreen_passes,
                     MetricId::render_texture_queries,
                     MetricId::render_copy_calls,
                     MetricId::render_fill_rect_calls,
                     MetricId::render_image_lookup_calls,
                     MetricId::render_time_ns,
                     MetricId::event_handler_invocations,
                     MetricId::events_bubbled,
                     MetricId::ui_style_allocations,
                     MetricId::ui_style_default_constructed,
                     MetricId::ui_style_copied,
                     MetricId::ui_style_assigned,
                     MetricId::ui_style_hover_allocations,
                     MetricId::ui_style_focus_within_allocations,
                     MetricId::ui_style_focus_allocations,
                     MetricId::style_resolve_calls,
                     MetricId::style_resolve_time_ns,
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_index_candidate_rules,
                     MetricId::theme_full_selector_match_checks,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_rejected,
                     MetricId::theme_rules_matched,
                     MetricId::style_view_cache_hits,
                     MetricId::style_view_cache_misses,
                     MetricId::style_view_resolutions,
                     MetricId::style_view_invalidations,
                     MetricId::style_view_property_lookup_calls,
                     MetricId::style_view_property_cache_hits,
                     MetricId::style_view_property_cache_misses,
                     MetricId::style_view_parent_steps,
                     MetricId::style_view_inherited_fallthroughs,
                     MetricId::text_render_op_creations,
                     MetricId::text_render_op_failures,
                     MetricId::text_measure_calls,
                     MetricId::text_measure_time_ns,
                     MetricId::text_line_measure_calls,
                     MetricId::text_layout_calls,
                     MetricId::text_layout_time_ns,
                     MetricId::text_render_calls,
                     MetricId::text_render_time_ns,
                     MetricId::text_render_lines,
                     MetricId::text_render_segments,
                     MetricId::text_renderer_size_calls,
                     MetricId::text_renderer_glyphs_measured,
                     MetricId::text_renderer_glyphs_rendered});
  }

  Category appfixture_category()
  {
    return category("appfixture",
                    {MetricId::runtime_push_mode_calls,
                     MetricId::runtime_pop_mode_calls,
                     MetricId::runtime_render_mode_calls,
                     MetricId::runtime_update_mode_calls,
                     MetricId::runtime_render_time_ns,
                     MetricId::runtime_update_time_ns,
                     MetricId::view_state_equality_checks,
                     MetricId::view_state_assignments_preserved,
                     MetricId::view_state_assignments_replaced,
                     MetricId::layout_view_tree_calls,
                     MetricId::layout_view_tree_nodes,
                     MetricId::layout_children_calls,
                     MetricId::layout_children_items,
                     MetricId::layout_content_size_hook_calls,
                     MetricId::layout_content_size_hook_time_ns,
                     MetricId::layout_natural_size_cache_hits,
                     MetricId::layout_natural_size_pass_cache_hits,
                     MetricId::layout_natural_size_persistent_cache_hits,
                     MetricId::layout_natural_size_persistent_cache_stores,
                     MetricId::layout_natural_size_cache_misses,
                     MetricId::layout_dependency_signature_calls,
                     MetricId::layout_dependency_signature_nodes,
                     MetricId::layout_dependency_signature_time_ns,
                     MetricId::layout_dirty_cache_hits,
                     MetricId::layout_dirty_cache_misses,
                     MetricId::layout_invalidations,
                     MetricId::layout_skipped_clean_subtrees,
                     MetricId::layout_dirty_subtree_nodes,
                     MetricId::layout_time_ns,
                     MetricId::render_view_calls,
                     MetricId::render_view_nodes,
                     MetricId::render_hook_calls,
                     MetricId::render_hook_time_ns,
                     MetricId::render_temporary_texture_creations,
                     MetricId::render_offscreen_passes,
                     MetricId::render_texture_queries,
                     MetricId::render_copy_calls,
                     MetricId::render_fill_rect_calls,
                     MetricId::render_image_lookup_calls,
                     MetricId::render_time_ns,
                     MetricId::event_handler_invocations,
                     MetricId::events_bubbled,
                     MetricId::ui_style_allocations,
                     MetricId::ui_style_default_constructed,
                     MetricId::ui_style_copied,
                     MetricId::ui_style_assigned,
                     MetricId::ui_style_hover_allocations,
                     MetricId::ui_style_focus_within_allocations,
                     MetricId::ui_style_focus_allocations,
                     MetricId::style_resolve_calls,
                     MetricId::style_resolve_time_ns,
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_index_candidate_rules,
                     MetricId::theme_full_selector_match_checks,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_rejected,
                     MetricId::theme_rules_matched,
                     MetricId::style_view_cache_hits,
                     MetricId::style_view_cache_misses,
                     MetricId::style_view_resolutions,
                     MetricId::style_view_invalidations,
                     MetricId::style_view_property_lookup_calls,
                     MetricId::style_view_property_cache_hits,
                     MetricId::style_view_property_cache_misses,
                     MetricId::style_view_parent_steps,
                     MetricId::style_view_inherited_fallthroughs,
                     MetricId::text_render_op_creations,
                     MetricId::text_render_op_failures,
                     MetricId::text_measure_calls,
                     MetricId::text_measure_time_ns,
                     MetricId::text_line_measure_calls,
                     MetricId::text_layout_calls,
                     MetricId::text_layout_time_ns,
                     MetricId::text_render_calls,
                     MetricId::text_render_time_ns,
                     MetricId::text_render_lines,
                     MetricId::text_render_segments,
                     MetricId::text_renderer_size_calls,
                     MetricId::text_renderer_glyphs_measured,
                     MetricId::text_renderer_glyphs_rendered});
  }

  bool category_enabled(const std::string& name)
  {
    if (active_config.category_filter.empty()) return true;
    for (const auto& enabled : active_config.category_filter)
    {
      if (enabled == name) return true;
    }
    return false;
  }

  void consume(std::int64_t value)
  {
    consumed_value ^= value;
  }

  Case::Case(std::string benchmark_name, Category category)
    : benchmark_name(std::move(benchmark_name))
    , category_spec(std::move(category))
  {
  }

  Case& Case::with_iterations(std::size_t iterations)
  {
    if (iterations == 0)
    {
      throw std::invalid_argument("benchmark iterations must be greater than zero");
    }
    this->iterations = iterations;
    return *this;
  }

  const Result& Case::run(const std::function<void()>& workload)
  {
    if (!category_enabled(category_spec.name))
    {
      last_result = Result{.benchmark_name = benchmark_name,
                           .category = category_spec,
                           .iterations = iterations};
      return last_result;
    }

    reset();
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iterations; i++)
    {
      workload();
    }
    const auto end = std::chrono::steady_clock::now();

    const auto elapsed = std::chrono::duration<double, std::milli>(end - start).count();

    last_result = Result{.benchmark_name = benchmark_name,
                         .category = category_spec,
                         .iterations = iterations,
                         .timestamp_ms = now_ms(),
                         .total_time_ms = elapsed,
                         .mean_time_ms = elapsed / static_cast<double>(iterations),
                         .metrics = snapshot_metrics()};

    print_result(last_result);
    if (active_config.csv_enabled)
    {
      write_csv_result(last_result);
    }
    consume(consumed_value);
    return last_result;
  }

  std::int64_t now_ms()
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
  }

  void validate_csv_output_allowed()
  {
    if (!active_config.csv_enabled || active_config.allow_dirty_worktree) return;

    const auto dirty = dirty_tracked_paths_in_pixils_library();
    if (dirty.empty()) return;

    std::ostringstream message;
    message << "Refusing to write benchmark CSVs with dirty tracked lib/pixils "
               "library files.\n"
            << "Use --no-csv for exploratory runs, or --allow-dirty-worktree "
               "if this is intentional.\n"
            << "Dirty tracked paths under lib/pixils, excluding "
               "lib/pixils/benchmark:\n";
    constexpr std::size_t max_paths = 12;
    for (std::size_t i = 0; i < dirty.size() && i < max_paths; i++)
    {
      message << "  " << dirty[i] << "\n";
    }
    if (dirty.size() > max_paths)
    {
      message << "  ... and " << (dirty.size() - max_paths) << " more\n";
    }
    throw std::runtime_error(message.str());
  }

  void print_result(const Result& result)
  {
    std::cout << "----------------------------------------------------\n";
    std::cout << result.benchmark_name << " [" << result.category.name << "]\n";
    std::cout << "+---------------------------------------------------\n";
    std::cout << "| * Total time: " << result.total_time_ms << " ms\n";
    std::cout << "| * Iterations: " << result.iterations << "\n";
    std::cout << "| * Time / iteration: " << result.mean_time_ms << " ms\n";
    for (MetricId id : result.category.metrics)
    {
      std::cout << "| * " << metric_name(id) << ": "
                << result.metrics[static_cast<std::size_t>(id)] << "\n";
    }
    std::cout << "+---------------------------------------------------\n";
  }

  void write_csv_result(const Result& result)
  {
    const auto dir = std::filesystem::path(active_config.output_dir) /
                     safe_path_part(active_config.goalpost) /
                     safe_path_part(result.category.name);
    const auto file_name = dir / (safe_path_part(result.benchmark_name) + ".csv");

    const std::string header = csv_header(result.category);
    const bool exists = std::filesystem::exists(file_name);
    bool write_header = true;
    {
      std::ifstream in(file_name);
      if (in.good())
      {
        std::string existing_header;
        std::getline(in, existing_header);
        write_header = existing_header.empty();
        if (!existing_header.empty() && existing_header != header)
        {
          throw std::runtime_error(
            "Benchmark CSV schema mismatch for " + file_name.string() +
            ". Start a new goalpost directory or migrate the file explicitly.");
        }
      }
    }

    std::filesystem::create_directories(dir);

    std::ofstream out(file_name, exists ? std::ios::app : std::ios::trunc);
    out << std::setprecision(12);

    if (write_header)
    {
      out << header << "\n";
    }

    out << result.timestamp_ms << "," << csv_field(active_config.goalpost) << ","
        << csv_field(active_config.run_name) << "," << csv_field(result.category.name) << ","
        << csv_field(result.benchmark_name) << "," << result.iterations << ","
        << result.total_time_ms << "," << result.mean_time_ms;
    for (MetricId id : result.category.metrics)
    {
      out << "," << result.metrics[static_cast<std::size_t>(id)];
    }
    out << "\n";
  }
} // namespace Pixils::Benchmark
