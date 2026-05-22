add_executable(matching_engine_test
  src/matching_engine_test.cpp
  src/order_book_test.cpp
)

target_link_libraries(matching_engine_test
  PRIVATE
    Catch2::Catch2WithMain
    matching_engine::matching_engine
)

catch_discover_tests(matching_engine_test
  TEST_PREFIX "matching_engine/"
)
