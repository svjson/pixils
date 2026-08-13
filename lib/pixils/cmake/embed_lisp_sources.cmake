if(NOT DEFINED PIXILS_LISP_ROOT)
  message(FATAL_ERROR "PIXILS_LISP_ROOT is required")
endif()
if(NOT DEFINED PIXILS_LISP_CPP)
  message(FATAL_ERROR "PIXILS_LISP_CPP is required")
endif()
if(NOT DEFINED PIXILS_LISP_H)
  message(FATAL_ERROR "PIXILS_LISP_H is required")
endif()

file(GLOB_RECURSE PIXILS_LISP_FILES RELATIVE "${PIXILS_LISP_ROOT}" "${PIXILS_LISP_ROOT}/*.roo")
list(SORT PIXILS_LISP_FILES)

get_filename_component(PIXILS_LISP_CPP_DIR "${PIXILS_LISP_CPP}" DIRECTORY)
get_filename_component(PIXILS_LISP_H_DIR "${PIXILS_LISP_H}" DIRECTORY)
file(MAKE_DIRECTORY "${PIXILS_LISP_CPP_DIR}")
file(MAKE_DIRECTORY "${PIXILS_LISP_H_DIR}")

set(HEADER_CONTENT [=[#ifndef PIXILS__EMBEDDED_LISP_SOURCES_H
#define PIXILS__EMBEDDED_LISP_SOURCES_H

#include <span>
#include <string_view>

#include <roo/io/embedded_file_system.h>

namespace Pixils::EmbeddedLisp
{
  using Source = Roo::EmbeddedFile;

  std::span<const Source> core_sources();
  std::span<const std::string_view> core_namespaces();
} // namespace Pixils::EmbeddedLisp

#endif
]=])

list(LENGTH PIXILS_LISP_FILES PIXILS_LISP_FILE_COUNT)

set(CPP_CONTENT [=[#include <pixils/embedded_lisp_sources.h>

#include <array>

namespace Pixils::EmbeddedLisp
{
  namespace
  {
]=])
string(APPEND CPP_CONTENT
  "    constexpr std::array<Source, ${PIXILS_LISP_FILE_COUNT}> CORE_SOURCES = {{\n")

set(NAMESPACE_CONTENT
  "    constexpr std::array<std::string_view, ${PIXILS_LISP_FILE_COUNT}> CORE_NAMESPACES = {{\n")

set(index 0)
foreach(rel_path IN LISTS PIXILS_LISP_FILES)
  file(READ "${PIXILS_LISP_ROOT}/${rel_path}" source_text)
  string(REGEX MATCH "\\(ns[ \t\r\n]+([^ \t\r\n()]+)" namespace_match "${source_text}")
  if(NOT namespace_match)
    message(FATAL_ERROR "Embedded Roo source has no namespace declaration: ${rel_path}")
  endif()
  set(namespace_name "${CMAKE_MATCH_1}")
  string(REPLACE "\\" "/" rel_path "${rel_path}")
  set(delim "PIXILS_EMBED_${index}")
  string(APPEND CPP_CONTENT "      {\"${rel_path}\", R\"${delim}(")
  string(APPEND CPP_CONTENT "${source_text}")
  string(APPEND CPP_CONTENT ")${delim}\"},\n")
  string(APPEND NAMESPACE_CONTENT "      \"${namespace_name}\",\n")
  math(EXPR index "${index} + 1")
endforeach()

string(APPEND CPP_CONTENT "    }};\n")
string(APPEND CPP_CONTENT "${NAMESPACE_CONTENT}")
string(APPEND CPP_CONTENT [=[    }};
  } // namespace

  std::span<const Source> core_sources()
  {
    return CORE_SOURCES;
  }

  std::span<const std::string_view> core_namespaces()
  {
    return CORE_NAMESPACES;
  }
} // namespace Pixils::EmbeddedLisp
]=])

file(WRITE "${PIXILS_LISP_H}" "${HEADER_CONTENT}")
file(WRITE "${PIXILS_LISP_CPP}" "${CPP_CONTENT}")
