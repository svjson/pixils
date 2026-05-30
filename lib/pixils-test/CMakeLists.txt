find_package(lisple-package REQUIRED)

add_library(pixils_test_lisple_native SHARED
  src/native/test_native.cpp
)

set_target_properties(pixils_test_lisple_native PROPERTIES
  OUTPUT_NAME pixils-test-native
  BUILD_RPATH "$<TARGET_FILE_DIR:pixils_shared>"
  INSTALL_RPATH "$ORIGIN/../../../../../lib"
)

target_link_libraries(pixils_test_lisple_native
  PUBLIC
    pixils_shared
    Lisple::lisple_shared
    Lisple::package_shared
)

add_custom_command(TARGET pixils_test_lisple_native POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/native
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:pixils_test_lisple_native>
          ${CMAKE_CURRENT_SOURCE_DIR}/native/$<TARGET_FILE_NAME:pixils_test_lisple_native>
  COMMENT "Copying Pixils test native package artifact into lib/pixils-test/native"
)

install(DIRECTORY src/
  DESTINATION share/lisple/pkg/pixils-test/src
  FILES_MATCHING
    PATTERN "*.lisple"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/lisple/pkg/pixils-test
)

install(TARGETS pixils_test_lisple_native
  LIBRARY DESTINATION share/lisple/pkg/pixils-test/native
  RUNTIME DESTINATION share/lisple/pkg/pixils-test/native
)
