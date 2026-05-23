if(NOT DEFINED PIXILS_LISP_ROOT)
  message(FATAL_ERROR "PIXILS_LISP_ROOT is required")
endif()
if(NOT DEFINED PIXILS_LISP_CPP)
  message(FATAL_ERROR "PIXILS_LISP_CPP is required")
endif()
if(NOT DEFINED PIXILS_LISP_H)
  message(FATAL_ERROR "PIXILS_LISP_H is required")
endif()

file(GLOB_RECURSE PIXILS_LISP_FILES RELATIVE "${PIXILS_LISP_ROOT}" "${PIXILS_LISP_ROOT}/*.lisple")
list(SORT PIXILS_LISP_FILES)

set(PIXILS_BOOTSTRAP_LISP_FILES
  ui/theme-assets.lisple
  ui/base-theme.lisple
  ui/button.lisple
  ui/window.lisple
  ui/dialog.lisple
  ui/scrollbar.lisple
  ui/scroll-pane.lisple
  ui/list-box.lisple
  ui/combo-box.lisple
  ui/text-input.lisple
  ui/number-input.lisple
  ui/file-dialog.lisple
)

foreach(bootstrap_file IN LISTS PIXILS_BOOTSTRAP_LISP_FILES)
  list(REMOVE_ITEM PIXILS_LISP_FILES "${bootstrap_file}")
endforeach()
list(PREPEND PIXILS_LISP_FILES ${PIXILS_BOOTSTRAP_LISP_FILES})

get_filename_component(PIXILS_LISP_CPP_DIR "${PIXILS_LISP_CPP}" DIRECTORY)
get_filename_component(PIXILS_LISP_H_DIR "${PIXILS_LISP_H}" DIRECTORY)
file(MAKE_DIRECTORY "${PIXILS_LISP_CPP_DIR}")
file(MAKE_DIRECTORY "${PIXILS_LISP_H_DIR}")

set(HEADER_CONTENT [=[#ifndef PIXILS__EMBEDDED_LISP_SOURCES_H
#define PIXILS__EMBEDDED_LISP_SOURCES_H

#include <vector>

namespace Pixils::EmbeddedLisp
{
  struct Source
  {
    const char* path;
    const char* source;
  };

  const std::vector<Source>& core_sources();
} // namespace Pixils::EmbeddedLisp

#endif
]=])

set(CPP_CONTENT [=[#include <pixils/embedded_lisp_sources.h>

namespace Pixils::EmbeddedLisp
{
  namespace
  {
    const std::vector<Source> CORE_SOURCES = {
]=])

set(index 0)
foreach(rel_path IN LISTS PIXILS_LISP_FILES)
  file(READ "${PIXILS_LISP_ROOT}/${rel_path}" source_text)
  string(REPLACE "\\" "/" rel_path "${rel_path}")
  set(delim "PIXILS_EMBED_${index}")
  string(APPEND CPP_CONTENT "      {\"${rel_path}\", R\"${delim}(")
  string(APPEND CPP_CONTENT "${source_text}")
  string(APPEND CPP_CONTENT ")${delim}\"},\n")
  math(EXPR index "${index} + 1")
endforeach()

string(APPEND CPP_CONTENT [=[    };
  } // namespace

  const std::vector<Source>& core_sources()
  {
    return CORE_SOURCES;
  }
} // namespace Pixils::EmbeddedLisp
]=])

file(WRITE "${PIXILS_LISP_H}" "${HEADER_CONTENT}")
file(WRITE "${PIXILS_LISP_CPP}" "${CPP_CONTENT}")
