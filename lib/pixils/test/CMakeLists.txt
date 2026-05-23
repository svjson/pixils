
include(GoogleTest)

find_package(sdl2_mock REQUIRED)

get_filename_component(PIXILS_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(PIXILS_TEST_LISPLE_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lisple")
set(PIXILS_RUNNER_PACKAGE_DIR "${PIXILS_REPO_ROOT}/lib/pixils-runner")
set(PIXILS_TEST_PACKAGE_DIR "${PIXILS_REPO_ROOT}/lib/pixils-test")
set(PIXILS_LISPLE_LOCAL_PACKAGE_ROOT "${PIXILS_REPO_ROOT}/../lisple-lang/pkg")
set(PIXILS_LISPLE_INSTALLED_PACKAGE_ROOT "$ENV{HOME}/.local/share/lisple/pkg")

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)

add_executable(testpixils ${TEST_SOURCES})
add_dependencies(testpixils pixils_lisple_native pixils_runner_lisple_native)

set_target_properties(testpixils PROPERTIES
  BUILD_RPATH "$<TARGET_FILE_DIR:pixils_lisple_native>"
  BUILD_RPATH_USE_LINK_PATH ON
)

target_include_directories(testpixils
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    ${SDL2_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
    ${SDL2MIXER_INCLUDE_DIRS}
)

target_link_libraries(testpixils
  gtest
  gmock
  pixils_objects
  Lisple::package_shared
  sdl2_mock::sdl2_mock
  Lisple::lisple_shared
)

target_compile_definitions(testpixils
  PRIVATE
    PIXILS_TEST_LISPLE_PACKAGE_DIR="${PIXILS_TEST_LISPLE_PACKAGE_DIR}"
    PIXILS_RUNNER_PACKAGE_DIR="${PIXILS_RUNNER_PACKAGE_DIR}"
    PIXILS_TEST_PACKAGE_DIR="${PIXILS_TEST_PACKAGE_DIR}"
    PIXILS_LISPLE_LOCAL_PACKAGE_ROOT="${PIXILS_LISPLE_LOCAL_PACKAGE_ROOT}"
    PIXILS_LISPLE_INSTALLED_PACKAGE_ROOT="${PIXILS_LISPLE_INSTALLED_PACKAGE_ROOT}"
)

gtest_discover_tests(testpixils
  WORKING_DIRECTORY "${PIXILS_REPO_ROOT}"
)
