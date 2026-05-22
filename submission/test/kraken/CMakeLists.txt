add_executable(kraken_test
  src/algorithm_test.cpp
  src/charconv_test.cpp
  src/expected_test.cpp
  src/fixed_string_test.cpp
  src/strong_type_test.cpp
)

target_link_libraries(kraken_test
  PRIVATE
    Catch2::Catch2WithMain
    kraken::kraken
)

catch_discover_tests(kraken_test
  TEST_PREFIX "kraken/"
)
