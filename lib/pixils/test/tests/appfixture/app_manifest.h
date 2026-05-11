#ifndef PIXILS__TEST__APPFIXTURE__APP_MANIFEST_H
#define PIXILS__TEST__APPFIXTURE__APP_MANIFEST_H

#include "app_source_builder.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Pixils::Test::AppFixture
{
  struct ManifestFile
  {
    std::string id;
    std::filesystem::path disk_path;
    std::string namespace_name;
    std::vector<std::string> unit_ids;
  };

  class AppManifest
  {
    std::vector<SourceUnit> unit_catalog;
    std::vector<ManifestFile> files;
    mutable std::optional<std::vector<ComposedFile>> materialized_files_cache;

    void invalidate_materialized_files();

   public:
    AppManifest() = default;
    AppManifest(std::vector<SourceUnit> units, std::vector<ManifestFile> files);

    void upsert_unit(const SourceUnit& unit);
    bool has_unit(const std::string& unit_id) const;
    void remove_unit(const std::string& unit_id);

    void add_file(const ManifestFile& file);
    bool has_file(const std::string& file_id) const;
    void remove_file(const std::string& file_id);

    void append_unit_to_file(const std::string& file_id, const std::string& unit_id);
    void insert_unit_after(const std::string& file_id,
                           const std::string& anchor_unit_id,
                           const std::string& unit_id);
    void remove_unit_from_file(const std::string& file_id, const std::string& unit_id);

    std::vector<ComposedFile> materialize_files() const;
  };
} // namespace Pixils::Test::AppFixture

#endif
