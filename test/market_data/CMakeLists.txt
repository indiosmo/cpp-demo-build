add_executable(market_data_test
  src/json_encoder_test.cpp
)

target_link_libraries(market_data_test
  PRIVATE
    Catch2::Catch2WithMain
    market_data::market_data
)

catch_discover_tests(market_data_test
  TEST_PREFIX "market_data/"
)
