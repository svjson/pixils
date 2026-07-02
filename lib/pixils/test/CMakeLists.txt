
include(GoogleTest)

find_package(sdl3_mock REQUIRED)

get_filename_component(PIXILS_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(PIXILS_TEST_ROO_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/roo")
set(PIXILS_RUNNER_PACKAGE_DIR "${PIXILS_REPO_ROOT}/lib/pixils-runner")
set(PIXILS_TEST_PACKAGE_DIR "${PIXILS_REPO_ROOT}/lib/pixils-test")
set(PIXILS_ROO_LOCAL_PACKAGE_ROOT "${PIXILS_REPO_ROOT}/../roo-lang/pkg")
set(PIXILS_ROO_INSTALLED_PACKAGE_ROOT "$ENV{HOME}/.local/share/roo/pkg")

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)

add_executable(testpixils ${TEST_SOURCES})
add_dependencies(testpixils pixils_roo_native pixils_runner_roo_native)

set_target_properties(testpixils PROPERTIES
  BUILD_RPATH "$<TARGET_FILE_DIR:pixils_roo_native>"
  BUILD_RPATH_USE_LINK_PATH ON
)

target_include_directories(testpixils
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)

target_link_libraries(testpixils
  gtest
  gmock
  pixils_objects
  pixils_clipper2
  Roo::package_shared
  sdl3_mock::sdl3_mock
  Roo::roo_shared
)

target_compile_definitions(testpixils
  PRIVATE
    SDL_ENABLE_OLD_NAMES
    SDL_INTERNAL
    PIXILS_TEST_ROO_PACKAGE_DIR="${PIXILS_TEST_ROO_PACKAGE_DIR}"
    PIXILS_RUNNER_PACKAGE_DIR="${PIXILS_RUNNER_PACKAGE_DIR}"
    PIXILS_TEST_PACKAGE_DIR="${PIXILS_TEST_PACKAGE_DIR}"
    PIXILS_ROO_LOCAL_PACKAGE_ROOT="${PIXILS_ROO_LOCAL_PACKAGE_ROOT}"
    PIXILS_ROO_INSTALLED_PACKAGE_ROOT="${PIXILS_ROO_INSTALLED_PACKAGE_ROOT}"
)

gtest_discover_tests(testpixils
  WORKING_DIRECTORY "${PIXILS_REPO_ROOT}"
  PROPERTIES
    LABELS core
)
