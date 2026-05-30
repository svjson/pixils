find_package(roo-package REQUIRED)

add_library(pixils_runner_roo_native SHARED
  src/native/runner_native.cpp
)

set_target_properties(pixils_runner_roo_native PROPERTIES
  OUTPUT_NAME pixils-runner-native
  BUILD_RPATH "$<TARGET_FILE_DIR:pixils_shared>"
  INSTALL_RPATH "$ORIGIN/../../../../../lib"
)

target_link_libraries(pixils_runner_roo_native
  PUBLIC
    pixils_shared
    Roo::roo_shared
    Roo::package_shared
)

add_custom_command(TARGET pixils_runner_roo_native POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/native
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:pixils_runner_roo_native>
          ${CMAKE_CURRENT_SOURCE_DIR}/native/$<TARGET_FILE_NAME:pixils_runner_roo_native>
  COMMENT "Copying Pixils runner native package artifact into lib/pixils-runner/native"
)

install(DIRECTORY src/
  DESTINATION share/roo/pkg/pixils-runner/src
  FILES_MATCHING
    PATTERN "*.roo"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/roo/pkg/pixils-runner
)

install(TARGETS pixils_runner_roo_native
  LIBRARY DESTINATION share/roo/pkg/pixils-runner/native
  RUNTIME DESTINATION share/roo/pkg/pixils-runner/native
)
