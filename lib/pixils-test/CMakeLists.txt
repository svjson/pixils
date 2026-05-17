install(DIRECTORY src/
  DESTINATION share/lisple/pkg/pixils-test/src
  FILES_MATCHING
    PATTERN "*.lisple"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/lisple/pkg/pixils-test
)
