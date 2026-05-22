add_executable(order_routing_test
  src/csv_decoder_test.cpp
)

target_link_libraries(order_routing_test
  PRIVATE
    Catch2::Catch2WithMain
    order_routing::order_routing
)

catch_discover_tests(order_routing_test
  TEST_PREFIX "order_routing/"
)
