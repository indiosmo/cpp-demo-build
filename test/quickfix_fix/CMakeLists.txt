add_executable(quickfix_fix_test
  src/codec_session_test.cpp
)

target_link_libraries(quickfix_fix_test
  PRIVATE
    Catch2::Catch2WithMain
    quickfix_fix::quickfix_fix
)

catch_discover_tests(quickfix_fix_test
  TEST_PREFIX "quickfix_fix/"
)
