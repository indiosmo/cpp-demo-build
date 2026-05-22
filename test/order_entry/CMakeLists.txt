add_executable(order_entry_test
  src/json_decoder_test.cpp
)

target_link_libraries(order_entry_test
  PRIVATE
    Catch2::Catch2WithMain
    order_entry::order_entry
)

catch_discover_tests(order_entry_test
  TEST_PREFIX "order_entry/"
)
