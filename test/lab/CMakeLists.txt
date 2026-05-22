add_executable(lab_test
  src/algorithm_test.cpp
  src/charconv_test.cpp
  src/expected_test.cpp
  src/fixed_string_test.cpp
  src/strong_type_test.cpp
)

target_link_libraries(lab_test
  PRIVATE
    Catch2::Catch2WithMain
    lab::core
)

catch_discover_tests(lab_test
  TEST_PREFIX "lab/"
)
