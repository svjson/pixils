
find_package(PkgConfig)
pkg_search_module(SDL2 sdl2)
pkg_search_module(SDL2IMAGE SDL2_image)
pkg_search_module(SDL2MIXER SDL2_mixer)
pkg_search_module(FREETYPE REQUIRED freetype2)
find_package(X11)
find_package(roo REQUIRED)
find_package(roo-package REQUIRED)

file(GLOB pixils_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
  src/asset/*.cpp
  src/binding/*.cpp
  src/binding/ui/*.cpp
  src/binding/ui/style/*.cpp
  src/runtime/*.cpp
  src/state/*.cpp
  src/ui/*.cpp
  src/ui/components/*.cpp
)
list(FILTER pixils_SOURCES EXCLUDE REGEX ".*/native_package\\.cpp$")

file(GLOB_RECURSE pixils_EMBEDDED_LISP_SOURCES CONFIGURE_DEPENDS
  lisp/*.roo
)

set(pixils_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(pixils_EMBEDDED_LISP_CPP ${pixils_GENERATED_DIR}/embedded_lisp_sources.cpp)
set(pixils_EMBEDDED_LISP_H ${pixils_GENERATED_DIR}/pixils/embedded_lisp_sources.h)

add_custom_command(
  OUTPUT ${pixils_EMBEDDED_LISP_CPP} ${pixils_EMBEDDED_LISP_H}
  COMMAND ${CMAKE_COMMAND}
          -DPIXILS_LISP_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/lisp
          -DPIXILS_LISP_CPP=${pixils_EMBEDDED_LISP_CPP}
          -DPIXILS_LISP_H=${pixils_EMBEDDED_LISP_H}
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_lisp_sources.cmake
  DEPENDS ${pixils_EMBEDDED_LISP_SOURCES}
          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_lisp_sources.cmake
  VERBATIM
)

add_library(pixils INTERFACE)
add_library(pixils_objects OBJECT ${pixils_SOURCES} ${pixils_EMBEDDED_LISP_CPP})
add_library(pixils_benchmark_objects OBJECT ${pixils_SOURCES} ${pixils_EMBEDDED_LISP_CPP})

foreach(target pixils_objects pixils_benchmark_objects)

  set_target_properties(${target} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )

  target_include_directories(${target}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${pixils_GENERATED_DIR}>
      $<INSTALL_INTERFACE:include>
      ${FREETYPE_INCLUDE_DIRS}
  )

  target_link_libraries(${target}
    PRIVATE
      SDL2
      SDL2_image
      SDL2_mixer
      ${FREETYPE_LIBRARIES}
      Roo::roo_shared
  )

endforeach()

target_compile_definitions(pixils_benchmark_objects
  PRIVATE
    PIXILS_ENABLE_BENCHMARK_COUNTERS
)

set_target_properties(pixils_benchmark_objects PROPERTIES
  EXCLUDE_FROM_ALL ON
)

add_library(pixils_static STATIC $<TARGET_OBJECTS:pixils_objects>)
add_library(pixils_shared SHARED $<TARGET_OBJECTS:pixils_objects>)

foreach(target pixils_static pixils_shared)

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME pixils
  )

  target_include_directories(${target}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${pixils_GENERATED_DIR}>
      $<INSTALL_INTERFACE:include>
      ${FREETYPE_INCLUDE_DIRS}
  )

endforeach()

target_link_libraries(pixils_shared
  PUBLIC
    SDL2
    SDL2_image
    SDL2_mixer
    ${FREETYPE_LIBRARIES}
    Roo::roo_shared
)

target_link_libraries(pixils_static
  PUBLIC
    SDL2
    SDL2_image
    SDL2_mixer
    ${FREETYPE_LIBRARIES}
    Roo::roo_static
)

add_library(pixils_roo_native SHARED
  $<TARGET_OBJECTS:pixils_objects>
  src/native_package.cpp
)

set_target_properties(pixils_roo_native PROPERTIES
  OUTPUT_NAME pixils-native
)

target_include_directories(pixils_roo_native
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${pixils_GENERATED_DIR}>
    $<INSTALL_INTERFACE:include>
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(pixils_roo_native
  PUBLIC
    SDL2
    SDL2_image
    SDL2_mixer
    ${FREETYPE_LIBRARIES}
    Roo::roo_shared
    Roo::package_shared
)

add_custom_command(TARGET pixils_roo_native POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/native
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:pixils_roo_native>
          ${CMAKE_CURRENT_SOURCE_DIR}/native/$<TARGET_FILE_NAME:pixils_roo_native>
  COMMENT "Copying Pixils Roo native package artifact into lib/pixils/native"
)

install(TARGETS pixils pixils_shared pixils_static pixils_roo_native
  EXPORT pixilsTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY include/
  DESTINATION include
)

install(EXPORT pixilsTargets
  NAMESPACE pixils::
  DESTINATION lib/cmake/pixils
)

install(FILES cmake/pixilsConfig.cmake
  DESTINATION lib/cmake/pixils
)

install(DIRECTORY lisp/
  DESTINATION share/roo/pkg/pixils/lisp
  FILES_MATCHING
    PATTERN "*.roo"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/roo/pkg/pixils
)

install(TARGETS pixils_roo_native
  LIBRARY DESTINATION share/roo/pkg/pixils/native
  RUNTIME DESTINATION share/roo/pkg/pixils/native
)

add_subdirectory(test)
add_subdirectory(benchmark)
