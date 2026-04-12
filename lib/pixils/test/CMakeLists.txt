
include(GoogleTest)

set(CMAKE_BUILD_TYPE Debug)

file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

add_executable(testpixils ${TEST_SOURCES})

target_link_libraries(testpixils
  gtest
  gmock
  pixils_static
)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)

gtest_discover_tests(testpixils)
