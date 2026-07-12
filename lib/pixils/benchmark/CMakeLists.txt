find_package(sdl3_mock REQUIRED)

file(GLOB_RECURSE PIXILS_BENCHMARK_SOURCES CONFIGURE_DEPENDS
  *.cpp
)

set(PIXILS_BENCHMARK_APPFIXTURE_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/../test/tests/input_simulator.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../test/tests/appfixture/app_manifest.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../test/tests/appfixture/app_source_builder.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../test/tests/appfixture/composable_app_session_fixture.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../test/tests/appfixture/minesweeper_app_manifest.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../test/tests/appfixture/tilemap_editor_app_manifest.cpp"
)

add_executable(benchmarkpixils EXCLUDE_FROM_ALL
  ${PIXILS_BENCHMARK_SOURCES}
  ${PIXILS_BENCHMARK_APPFIXTURE_SOURCES}
  $<TARGET_OBJECTS:pixils_clipper2>
)
add_dependencies(benchmarkpixils pixils_roo_native)

set_target_properties(benchmarkpixils PROPERTIES
  BUILD_RPATH "$<TARGET_FILE_DIR:pixils_roo_native>"
  BUILD_RPATH_USE_LINK_PATH ON
)

target_include_directories(benchmarkpixils
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
    "${CMAKE_CURRENT_SOURCE_DIR}/../test/tests"
)

target_compile_definitions(benchmarkpixils
  PRIVATE
    SDL_ENABLE_OLD_NAMES
    SDL_INTERNAL
    PIXILS_ENABLE_BENCHMARK_COUNTERS
)

target_link_libraries(benchmarkpixils
  gtest
  gmock
  pixils_benchmark_objects
  Roo::package_shared
  Roo::roo_shared
  sdl3_mock::sdl3_mock
  ${FREETYPE_LIBRARIES}
)
