add_executable(mmd_json_test
  src/json_encoder_test.cpp
)

target_link_libraries(mmd_json_test
  PRIVATE
    Catch2::Catch2WithMain
    mmd_json::mmd_json
)

catch_discover_tests(mmd_json_test
  TEST_PREFIX "mmd_json/"
)
