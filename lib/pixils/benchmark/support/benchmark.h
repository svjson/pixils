#ifndef PIXILS__BENCHMARK__BENCHMARK_H
#define PIXILS__BENCHMARK__BENCHMARK_H

#include <pixils/benchmark/counters.h>

#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

namespace Pixils::Benchmark
{
  inline constexpr std::size_t METRIC_COUNT = static_cast<std::size_t>(MetricId::count);
  using MetricValues = std::array<std::int64_t, METRIC_COUNT>;

  struct Config
  {
    bool csv_enabled = true;
    bool allow_dirty_worktree = false;
    std::string output_dir = "benchmarks/pixils";
    // Change this manually when starting a new benchmark goalpost.
    std::string goalpost = "005_usage_patterns_baseline_808e9fd";
    std::string run_name = "history";
    std::vector<std::string> category_filter;
  };

  struct Category
  {
    std::string name;
    std::vector<MetricId> metrics;
  };

  struct Result
  {
    std::string benchmark_name;
    Category category;
    std::size_t iterations = 1;
    std::int64_t timestamp_ms = 0;
    double total_time_ms = 0.0;
    double mean_time_ms = 0.0;
    MetricValues metrics{};
  };

  Config& config();

  Category category(std::string name, std::initializer_list<MetricId> metrics);
  Category layout_category();
  Category render_category();
  Category runtime_category();
  Category ui_category();
  Category appfixture_category();

  std::string_view metric_name(MetricId id);
  bool category_enabled(const std::string& name);
  void consume(std::int64_t value);

  class Case
  {
   public:
    explicit Case(std::string benchmark_name, Category category);

    Case& with_iterations(std::size_t iterations);
    const Result& run(const std::function<void()>& workload);

   private:
    std::string benchmark_name;
    Category category_spec;
    std::size_t iterations = 1;
    Result last_result;
  };

  std::int64_t now_ms();
  void validate_csv_output_allowed();
  void print_result(const Result& result);
  void write_csv_result(const Result& result);
} // namespace Pixils::Benchmark

#endif /* PIXILS__BENCHMARK__BENCHMARK_H */
