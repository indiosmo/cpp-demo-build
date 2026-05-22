add_executable(ospec_test
  src/b3_test.cpp
)

target_link_libraries(ospec_test
  PRIVATE
    Catch2::Catch2WithMain
    ospec::ospec
)

catch_discover_tests(ospec_test
  TEST_PREFIX "ospec/"
)
