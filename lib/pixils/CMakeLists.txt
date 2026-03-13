
file(GLOB pixils_SOURCES
  src/*.cpp
  src/binding/*.cpp
  src/runtime/*.cpp
)

add_library(pixils ${pixils_SOURCES})

find_package(PkgConfig)
pkg_search_module(SDL2 sdl2)
pkg_search_module(SDL2IMAGE SDL2_image)
pkg_search_module(SDL2MIXER SDL2_mixer)
find_package(X11)

target_include_directories(pixils
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(pixils
  PUBLIC
    SDL2
    SDL2_image
    SDL2_mixer
    lisple
)
