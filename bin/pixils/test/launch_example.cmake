if (NOT DEFINED PIXILS_BIN)
  message(FATAL_ERROR "PIXILS_BIN is required")
endif()

if (NOT DEFINED PIXILS_REPO_ROOT)
  message(FATAL_ERROR "PIXILS_REPO_ROOT is required")
endif()

if (NOT DEFINED PIXILS_EXAMPLE_PATH)
  message(FATAL_ERROR "PIXILS_EXAMPLE_PATH is required")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}" -E env
      SDL_VIDEODRIVER=dummy
      SDL_AUDIODRIVER=dummy
      "${PIXILS_BIN}" "${PIXILS_EXAMPLE_PATH}"
  WORKING_DIRECTORY "${PIXILS_REPO_ROOT}"
  TIMEOUT 1.5
  RESULT_VARIABLE result
)

if (result MATCHES "[Tt]imeout")
  message(STATUS "Example stayed alive until timeout: ${PIXILS_EXAMPLE_PATH}")
  return()
endif()

if (result EQUAL 0)
  message(FATAL_ERROR "Example exited before timeout: ${PIXILS_EXAMPLE_PATH}")
endif()

message(FATAL_ERROR "Example failed to launch: ${PIXILS_EXAMPLE_PATH}, result=${result}")
