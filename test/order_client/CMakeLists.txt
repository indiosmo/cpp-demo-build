add_executable(order_client_test
  src/csv_encoder_test.cpp
)

target_link_libraries(order_client_test
  PRIVATE
    Catch2::Catch2WithMain
    order_client::order_client
)

catch_discover_tests(order_client_test
  TEST_PREFIX "order_client/"
)
