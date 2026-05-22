add_executable(morfix_test
  src/conversions_test.cpp
  src/session_test.cpp
)

target_link_libraries(morfix_test
  PRIVATE
    Catch2::Catch2WithMain
    morfix::morfix
)

catch_discover_tests(morfix_test
  TEST_PREFIX "morfix/"
)
