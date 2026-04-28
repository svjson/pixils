#include "app_manifest.h"

#include <stdexcept>

namespace Pixils::Test::AppFixture
{
  namespace
  {
    SourceUnit* find_unit(std::vector<SourceUnit>& units, const std::string& unit_id)
    {
      for (auto& unit : units)
      {
        if (unit.id == unit_id) return &unit;
      }
      return nullptr;
    }

    const SourceUnit* find_unit(const std::vector<SourceUnit>& units,
                                const std::string& unit_id)
    {
      for (const auto& unit : units)
      {
        if (unit.id == unit_id) return &unit;
      }
      return nullptr;
    }

    ManifestFile* find_file(std::vector<ManifestFile>& files, const std::string& file_id)
    {
      for (auto& file : files)
      {
        if (file.id == file_id) return &file;
      }
      return nullptr;
    }

    const ManifestFile* find_file(const std::vector<ManifestFile>& files,
                                  const std::string& file_id)
    {
      for (const auto& file : files)
      {
        if (file.id == file_id) return &file;
      }
      return nullptr;
    }
  } // namespace

  AppManifest::AppManifest(std::vector<SourceUnit> units, std::vector<ManifestFile> files)
    : unit_catalog(std::move(units))
    , files(std::move(files))
  {
  }

  void AppManifest::upsert_unit(const SourceUnit& unit)
  {
    if (auto* existing = find_unit(unit_catalog, unit.id))
    {
      *existing = unit;
      return;
    }

    unit_catalog.push_back(unit);
  }

  bool AppManifest::has_unit(const std::string& unit_id) const
  {
    return find_unit(unit_catalog, unit_id) != nullptr;
  }

  void AppManifest::remove_unit(const std::string& unit_id)
  {
    bool found = false;
    for (auto it = unit_catalog.begin(); it != unit_catalog.end(); ++it)
    {
      if (it->id == unit_id)
      {
        unit_catalog.erase(it);
        found = true;
        break;
      }
    }

    if (!found) throw std::runtime_error("Unknown unit id: " + unit_id);

    for (auto& file : files)
    {
      for (auto it = file.unit_ids.begin(); it != file.unit_ids.end();)
      {
        if (*it == unit_id)
          it = file.unit_ids.erase(it);
        else
          ++it;
      }
    }
  }

  void AppManifest::add_file(const ManifestFile& file)
  {
    if (has_file(file.id)) throw std::runtime_error("Duplicate file id: " + file.id);
    files.push_back(file);
  }

  bool AppManifest::has_file(const std::string& file_id) const
  {
    return find_file(files, file_id) != nullptr;
  }

  void AppManifest::append_unit_to_file(const std::string& file_id, const std::string& unit_id)
  {
    auto* file = find_file(files, file_id);
    if (!file) throw std::runtime_error("Unknown file id: " + file_id);
    if (!has_unit(unit_id)) throw std::runtime_error("Unknown unit id: " + unit_id);

    file->unit_ids.push_back(unit_id);
  }

  void AppManifest::insert_unit_after(const std::string& file_id,
                                      const std::string& anchor_unit_id,
                                      const std::string& unit_id)
  {
    auto* file = find_file(files, file_id);
    if (!file) throw std::runtime_error("Unknown file id: " + file_id);
    if (!has_unit(unit_id)) throw std::runtime_error("Unknown unit id: " + unit_id);

    for (auto it = file->unit_ids.begin(); it != file->unit_ids.end(); ++it)
    {
      if (*it == anchor_unit_id)
      {
        file->unit_ids.insert(it + 1, unit_id);
        return;
      }
    }

    throw std::runtime_error("Unknown anchor unit id '" + anchor_unit_id +
                             "' in file '" + file_id + "'");
  }

  void AppManifest::remove_unit_from_file(const std::string& file_id,
                                          const std::string& unit_id)
  {
    auto* file = find_file(files, file_id);
    if (!file) throw std::runtime_error("Unknown file id: " + file_id);

    for (auto it = file->unit_ids.begin(); it != file->unit_ids.end(); ++it)
    {
      if (*it == unit_id)
      {
        file->unit_ids.erase(it);
        return;
      }
    }

    throw std::runtime_error("Unknown unit id '" + unit_id + "' in file '" + file_id + "'");
  }

  std::vector<ComposedFile> AppManifest::materialize_files() const
  {
    std::vector<ComposedFile> out;
    out.reserve(files.size());

    for (const auto& file : files)
    {
      ComposedFile composed{
        .disk_path = file.disk_path,
        .namespace_name = file.namespace_name,
        .units = {}};

      for (const auto& unit_id : file.unit_ids)
      {
        const auto* unit = find_unit(unit_catalog, unit_id);
        if (!unit)
          throw std::runtime_error("File '" + file.id + "' references unknown unit id '" +
                                   unit_id + "'");

        composed.units.push_back(*unit);
      }

      out.push_back(std::move(composed));
    }

    return out;
  }
} // namespace Pixils::Test::AppFixture
