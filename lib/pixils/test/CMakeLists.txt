
include(GoogleTest)

set(CMAKE_BUILD_TYPE Debug)

find_package(sdl2_mock REQUIRED)

file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

add_executable(testpixils ${TEST_SOURCES})

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
  sdl2_mock::sdl2_mock
  Lisple::lisple_shared
)

gtest_discover_tests(testpixils)
