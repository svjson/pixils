
add_executable(pixils_bin src/main.cpp)

find_package(roo-package REQUIRED)

set_target_properties(pixils_bin PROPERTIES OUTPUT_NAME pixils)

target_link_libraries(pixils_bin PRIVATE pixils_static Roo::package_static)

install(TARGETS pixils_bin RUNTIME DESTINATION bin)

set(PIXILS_EXAMPLE_APPS
  asteroids
  minesweeper
  spaceshooter
  tilemap-editor
  workbench
)

foreach(example IN LISTS PIXILS_EXAMPLE_APPS)
  add_test(
    NAME pixils_launch_${example}
    COMMAND ${CMAKE_COMMAND}
      -DPIXILS_BIN=$<TARGET_FILE:pixils_bin>
      -DPIXILS_REPO_ROOT=${CMAKE_SOURCE_DIR}
      -DPIXILS_EXAMPLE_PATH=examples/${example}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/test/launch_example.cmake
  )
  set_tests_properties(pixils_launch_${example}
    PROPERTIES
      LABELS core
  )
endforeach()
