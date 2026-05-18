
add_executable(pixils_bin src/main.cpp)

find_package(lisple-package REQUIRED)

set_target_properties(pixils_bin PROPERTIES OUTPUT_NAME pixils)

target_link_libraries(pixils_bin PRIVATE pixils_static Lisple::package_static)

install(TARGETS pixils_bin RUNTIME DESTINATION bin)
