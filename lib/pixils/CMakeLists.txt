
find_package(PkgConfig)
pkg_search_module(SDL2 sdl2)
pkg_search_module(SDL2IMAGE SDL2_image)
pkg_search_module(SDL2MIXER SDL2_mixer)
find_package(X11)
find_package(lisple REQUIRED)

file(GLOB pixils_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
  src/asset/*.cpp
  src/binding/*.cpp
  src/runtime/*.cpp
  src/ui/*.cpp
)

add_library(pixils INTERFACE)
add_library(pixils_objects OBJECT ${pixils_SOURCES})

set_target_properties(pixils_objects PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(pixils_objects
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(pixils_objects
  PRIVATE
    SDL2
    SDL2_image
    SDL2_mixer
    Lisple::lisple_shared
)

add_library(pixils_static STATIC $<TARGET_OBJECTS:pixils_objects>)
add_library(pixils_shared SHARED $<TARGET_OBJECTS:pixils_objects>)

foreach(target pixils_static pixils_shared)

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME pixils
  )

  target_include_directories(${target}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )

endforeach()

target_link_libraries(pixils_shared
  PUBLIC
    SDL2
    SDL2_image
    SDL2_mixer
    Lisple::lisple_shared
)

target_link_libraries(pixils_static
  PUBLIC
    SDL2
    SDL2_image
    SDL2_mixer
    Lisple::lisple_static
)

install(TARGETS pixils pixils_shared pixils_static
  EXPORT pixilsTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY include/
  DESTINATION include
)

install(EXPORT pixilsTargets
  NAMESPACE pixils::
  DESTINATION lib/cmake/pixils
)

install(FILES cmake/pixilsConfig.cmake
  DESTINATION lib/cmake/pixils
)

add_subdirectory(test)
