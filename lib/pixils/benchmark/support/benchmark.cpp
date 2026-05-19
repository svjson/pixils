#include "benchmark.h"

#include <chrono>
#include <algorithm>
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

    std::size_t csv_field_count(const std::string& line)
    {
      return static_cast<std::size_t>(
               std::count(line.begin(), line.end(), ',')) +
             1;
    }

    void ensure_csv_schema(const std::filesystem::path& file_name,
                           const std::string& expected_header)
    {
      std::ifstream in(file_name);
      if (!in.good()) return;

      std::vector<std::string> lines;
      std::string line;
      while (std::getline(in, line))
      {
        lines.push_back(line);
      }
      in.close();

      if (lines.empty() || lines.front() == expected_header) return;

      const auto old_field_count = csv_field_count(lines.front());
      const auto new_field_count = csv_field_count(expected_header);
      lines.front() = expected_header;

      if (old_field_count < new_field_count)
      {
        const std::string padding(new_field_count - old_field_count, ',');
        for (std::size_t i = 1; i < lines.size(); i++)
        {
          lines[i] += padding;
        }
      }

      std::ofstream out(file_name, std::ios::trunc);
      for (const auto& output_line : lines)
      {
        out << output_line << "\n";
      }
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
                     MetricId::layout_natural_size_cache_hits,
                     MetricId::ui_style_allocations,
                     MetricId::ui_style_default_constructed,
                     MetricId::ui_style_copied,
                     MetricId::ui_style_assigned,
                     MetricId::ui_style_hover_allocations,
                     MetricId::ui_style_focus_within_allocations,
                     MetricId::ui_style_focus_allocations,
                     MetricId::style_resolve_calls,
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_matched});
  }

  Category render_category()
  {
    return category("render",
                    {MetricId::render_view_calls,
                     MetricId::render_view_nodes,
                     MetricId::render_hook_calls,
                     MetricId::layout_view_tree_calls,
                     MetricId::layout_view_tree_nodes,
                     MetricId::ui_style_allocations,
                     MetricId::ui_style_default_constructed,
                     MetricId::ui_style_copied,
                     MetricId::ui_style_assigned,
                     MetricId::ui_style_hover_allocations,
                     MetricId::ui_style_focus_within_allocations,
                     MetricId::ui_style_focus_allocations,
                     MetricId::style_resolve_calls,
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_matched});
  }

  Category runtime_category()
  {
    return category("runtime",
                    {MetricId::runtime_push_mode_calls,
                     MetricId::runtime_pop_mode_calls,
                     MetricId::runtime_render_mode_calls,
                     MetricId::runtime_update_mode_calls,
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
                     MetricId::render_view_calls,
                     MetricId::render_view_nodes,
                     MetricId::render_hook_calls,
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
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_matched});
  }

  Category appfixture_category()
  {
    return category("appfixture",
                    {MetricId::runtime_push_mode_calls,
                     MetricId::runtime_pop_mode_calls,
                     MetricId::runtime_render_mode_calls,
                     MetricId::runtime_update_mode_calls,
                     MetricId::layout_view_tree_calls,
                     MetricId::layout_view_tree_nodes,
                     MetricId::layout_children_calls,
                     MetricId::layout_children_items,
                     MetricId::layout_content_size_hook_calls,
                     MetricId::layout_natural_size_cache_hits,
                     MetricId::render_view_calls,
                     MetricId::render_view_nodes,
                     MetricId::render_hook_calls,
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
                     MetricId::style_variant_apply_calls,
                     MetricId::style_layer_resolve_calls,
                     MetricId::runtime_style_source_resolve_calls,
                     MetricId::theme_matching_calls,
                     MetricId::theme_rule_match_checks,
                     MetricId::theme_rules_matched});
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

    const auto elapsed =
      std::chrono::duration<double, std::milli>(end - start).count();

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

    std::filesystem::create_directories(dir);

    const std::string header = csv_header(result.category);

    bool empty = true;
    {
      std::ifstream in(file_name);
      if (in.good())
      {
        empty = in.peek() == std::ifstream::traits_type::eof();
      }
    }
    if (!empty)
    {
      ensure_csv_schema(file_name, header);
    }

    std::ofstream out(file_name, std::ios::app);
    out << std::setprecision(12);

    if (empty)
    {
      out << header << "\n";
    }

    out << result.timestamp_ms << "," << csv_field(active_config.goalpost) << ","
        << csv_field(active_config.run_name) << ","
        << csv_field(result.category.name) << ","
        << csv_field(result.benchmark_name) << "," << result.iterations << ","
        << result.total_time_ms << "," << result.mean_time_ms;
    for (MetricId id : result.category.metrics)
    {
      out << "," << result.metrics[static_cast<std::size_t>(id)];
    }
    out << "\n";
  }
} // namespace Pixils::Benchmark
