#include "support/benchmark.h"

#include <gtest/gtest.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
  bool has_value(int index, int argc, const std::string& flag)
  {
    if (index + 1 < argc) return true;
    throw std::invalid_argument(flag + " requires a value");
  }
} // namespace

int main(int argc, char** argv)
{
  std::vector<char*> gtest_args;
  gtest_args.push_back(argv[0]);

  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg == "--benchmark" || arg == "--csv")
    {
      Pixils::Benchmark::config().csv_enabled = true;
    }
    else if (arg == "--no-csv")
    {
      Pixils::Benchmark::config().csv_enabled = false;
    }
    else if (arg == "--allow-dirty-worktree" || arg == "--allow-dirty")
    {
      Pixils::Benchmark::config().allow_dirty_worktree = true;
    }
    else if (arg == "--benchmark-dir")
    {
      has_value(i, argc, arg);
      Pixils::Benchmark::config().output_dir = argv[++i];
    }
    else if (arg == "--benchmark-goalpost")
    {
      has_value(i, argc, arg);
      Pixils::Benchmark::config().goalpost = argv[++i];
    }
    else if (arg == "--benchmark-run")
    {
      has_value(i, argc, arg);
      Pixils::Benchmark::config().run_name = argv[++i];
    }
    else if (arg == "--benchmark-category")
    {
      has_value(i, argc, arg);
      Pixils::Benchmark::config().category_filter.push_back(argv[++i]);
    }
    else
    {
      gtest_args.push_back(argv[i]);
    }
  }

  int gtest_argc = static_cast<int>(gtest_args.size());
  ::testing::InitGoogleTest(&gtest_argc, gtest_args.data());
  try
  {
    Pixils::Benchmark::validate_csv_output_allowed();
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << "\n";
    return 1;
  }
  return RUN_ALL_TESTS();
}
