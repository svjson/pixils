#ifndef PIXILS__TEST__APPFIXTURE__APP_SOURCE_BUILDER_H
#define PIXILS__TEST__APPFIXTURE__APP_SOURCE_BUILDER_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Pixils::Test::AppFixture
{
  struct SourceUnit
  {
    std::string id;
    std::vector<std::string> require_entries;
    std::vector<std::string> body_forms;

    static SourceUnit from_source(const std::string& id, const std::string& source_text);
    static SourceUnit from_file(const std::string& id, const std::filesystem::path& file_path);
    static SourceUnit inline_unit(const std::string& id,
                                  const std::vector<std::string>& require_entries,
                                  const std::vector<std::string>& body_forms);
  };

  struct ComposedFile
  {
    std::filesystem::path disk_path;
    std::string namespace_name;
    std::vector<SourceUnit> units;
  };

  std::string compose_file_content(const ComposedFile& file);
  void write_composed_file(const ComposedFile& file, const std::filesystem::path& root_dir);
} // namespace Pixils::Test::AppFixture

#endif
