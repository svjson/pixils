#include "app_source_builder.h"

#include <fstream>
#include <iostream>
#include <lisple/form.h>
#include <lisple/reader.h>
#include <lisple/type.h>
#include <sstream>
#include <stdexcept>

namespace Pixils::Test::AppFixture
{
  namespace
  {
    std::string read_file_to_string(const std::filesystem::path& file_path)
    {
      std::ifstream stream(file_path);
      if (!stream.is_open())
        throw std::runtime_error("Failed to open source unit file: " + file_path.string());

      std::ostringstream buffer;
      buffer << stream.rdbuf();
      return buffer.str();
    }

    const Lisple::List& as_list(const Lisple::sptr_sobject& form, const char* what)
    {
      if (!form || form->get_type() != Lisple::Form::LIST)
        throw std::runtime_error(std::string(what) + " must be a list form");
      return form->as<Lisple::List>();
    }

    bool is_word(const Lisple::sptr_sobject& form, const std::string& value)
    {
      return form && form->get_type() == Lisple::Form::WORD &&
             form->as<Lisple::Word>().value == value;
    }

    bool is_key(const Lisple::sptr_sobject& form, const std::string& value)
    {
      return form && form->get_type() == Lisple::Form::KEY && form->to_string() == value;
    }

    std::vector<std::string> parse_require_entries(const Lisple::List& ns_form)
    {
      std::vector<std::string> require_entries;

      for (size_t i = 2; i < ns_form.children.size(); i++)
      {
        const auto& clause = ns_form.children[i];
        if (!clause || clause->get_type() != Lisple::Form::LIST) continue;

        const auto& clause_list = clause->as<Lisple::List>();
        if (clause_list.children.empty()) continue;
        if (!is_key(clause_list.children.front(), ":require")) continue;

        for (size_t j = 1; j < clause_list.children.size(); j++)
          require_entries.push_back(clause_list.children[j]->to_string());
      }

      return require_entries;
    }

    std::vector<std::string> dedupe_require_entries(const std::vector<SourceUnit>& units)
    {
      std::vector<std::string> merged;

      for (const auto& unit : units)
      {
        for (const auto& entry : unit.require_entries)
        {
          bool exists = false;
          for (const auto& seen : merged)
          {
            if (seen == entry)
            {
              exists = true;
              break;
            }
          }
          if (!exists) merged.push_back(entry);
        }
      }

      return merged;
    }

  } // namespace

  SourceUnit SourceUnit::from_source(const std::string& id, const std::string& source_text)
  {
    Lisple::Reader reader;
    auto forms = reader.read_sexps(source_text);
    if (forms.empty()) throw std::runtime_error("Source unit '" + id + "' is empty");

    const auto& ns_form = as_list(forms.front(), "First top-level form");
    if (ns_form.children.empty() || !is_word(ns_form.children.front(), "ns"))
      throw std::runtime_error("Source unit '" + id + "' must start with an ns form");

    SourceUnit unit;
    unit.id = id;
    unit.require_entries = parse_require_entries(ns_form);

    for (size_t i = 1; i < forms.size(); i++)
      unit.body_forms.push_back(forms[i]->to_string());

    return unit;
  }

  SourceUnit SourceUnit::from_file(const std::string& id,
                                   const std::filesystem::path& file_path)
  {
    return from_source(id, read_file_to_string(file_path));
  }

  SourceUnit SourceUnit::inline_unit(const std::string& id,
                                     const std::vector<std::string>& require_entries,
                                     const std::vector<std::string>& body_forms)
  {
    SourceUnit unit;
    unit.id = id;
    unit.require_entries = require_entries;
    unit.body_forms = body_forms;
    return unit;
  }

  std::string compose_file_content(const ComposedFile& file)
  {
    std::ostringstream out;
    out << "(ns " << file.namespace_name;

    auto require_entries = dedupe_require_entries(file.units);
    if (!require_entries.empty())
    {
      out << "\n  (:require";
      for (const auto& entry : require_entries)
        out << " " << entry;
      out << ")";
    }
    out << ")\n";

    bool wrote_body = false;
    for (const auto& unit : file.units)
    {
      for (const auto& form : unit.body_forms)
      {
        out << "\n" << form << "\n";
        wrote_body = true;
      }
    }

    if (!wrote_body) out << "\n";

    return out.str();
  }

  void write_composed_file(const ComposedFile& file, const std::filesystem::path& root_dir)
  {
    auto out_path = root_dir / file.disk_path;
    std::filesystem::create_directories(out_path.parent_path());

    std::ofstream stream(out_path);
    if (!stream.is_open())
      throw std::runtime_error("Failed to write composed file: " + out_path.string());

    stream << compose_file_content(file);
  }
} // namespace Pixils::Test::AppFixture
