add_executable(morfix_quickfix_test
  src/codecs_test.cpp
)

target_link_libraries(morfix_quickfix_test
  PRIVATE
    Catch2::Catch2WithMain
    morfix_quickfix::morfix_quickfix
)

catch_discover_tests(morfix_quickfix_test
  TEST_PREFIX "morfix_quickfix/"
)
