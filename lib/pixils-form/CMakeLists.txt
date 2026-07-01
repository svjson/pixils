install(DIRECTORY src/
  DESTINATION share/roo/pkg/pixils-form/src
  FILES_MATCHING
    PATTERN "*.roo"
    PATTERN "*.lspl"
)

install(FILES package.edn
  DESTINATION share/roo/pkg/pixils-form
)
