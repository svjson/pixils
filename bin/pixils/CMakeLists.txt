
add_executable(pixils_bin src/main.cpp)

set_target_properties(pixils_bin PROPERTIES OUTPUT_NAME pixils)

target_link_libraries(pixils_bin PRIVATE pixils_static)

install(TARGETS pixils_bin RUNTIME DESTINATION bin)
