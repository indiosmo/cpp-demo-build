project(order_entry VERSION 0.1.0
  DESCRIPTION "Matching Engine Lab order-entry domain library"
  LANGUAGES CXX)

add_library(order_entry STATIC
  src/csv_decoder.cpp
  src/session.cpp
  src/runtime/session.cpp
)
add_library(order_entry::order_entry ALIAS order_entry)

target_include_directories(order_entry
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# The runtime composer owns the UDP receiver backends and threads a
# caller-provided boost::asio::io_context through to the asio receiver,
# so it pulls in the asio network target. The pure domain code still
# only depends on lab::core.
target_link_libraries(order_entry
  PUBLIC
    lab::core
    lab::network::asio
    lab::network::ef_vi
)
