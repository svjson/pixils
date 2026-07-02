install(DIRECTORY src/
  DESTINATION share/roo/pkg/pixils-tilemap-project/src
  FILES_MATCHING
    PATTERN "*.roo"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/roo/pkg/pixils-tilemap-project
)
