install(DIRECTORY src/
  DESTINATION share/lisple/pkg/pixils-runner/src
  FILES_MATCHING
    PATTERN "*.lisple"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/lisple/pkg/pixils-runner
)
