add_executable(mor_test
  src/conversions_test.cpp
  src/interfaces_test.cpp
)

target_link_libraries(mor_test
  PRIVATE
    Catch2::Catch2WithMain
    mor::mor
)

catch_discover_tests(mor_test
  TEST_PREFIX "mor/"
)
