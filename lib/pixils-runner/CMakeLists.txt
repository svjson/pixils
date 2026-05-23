find_package(lisple-package REQUIRED)

add_library(pixils_runner_lisple_native SHARED
  src/native/runner_native.cpp
)

set_target_properties(pixils_runner_lisple_native PROPERTIES
  OUTPUT_NAME pixils-runner-native
  BUILD_RPATH "$<TARGET_FILE_DIR:pixils_shared>"
  INSTALL_RPATH "$ORIGIN/../../../../../lib"
)

target_link_libraries(pixils_runner_lisple_native
  PUBLIC
    pixils_shared
    Lisple::lisple_shared
    Lisple::package_shared
)

add_custom_command(TARGET pixils_runner_lisple_native POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/native
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:pixils_runner_lisple_native>
          ${CMAKE_CURRENT_SOURCE_DIR}/native/$<TARGET_FILE_NAME:pixils_runner_lisple_native>
  COMMENT "Copying Pixils runner native package artifact into lib/pixils-runner/native"
)

install(DIRECTORY src/
  DESTINATION share/lisple/pkg/pixils-runner/src
  FILES_MATCHING
    PATTERN "*.lisple"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/lisple/pkg/pixils-runner
)

install(TARGETS pixils_runner_lisple_native
  LIBRARY DESTINATION share/lisple/pkg/pixils-runner/native
  RUNTIME DESTINATION share/lisple/pkg/pixils-runner/native
)
