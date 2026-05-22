project(order_routing VERSION 0.1.0
  DESCRIPTION "Kraken senior submission order routing domain library"
  LANGUAGES CXX)

add_library(order_routing STATIC
  src/csv_decoder.cpp
  src/session.cpp
  src/runtime/session.cpp
)
add_library(order_routing::order_routing ALIAS order_routing)

target_include_directories(order_routing
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# The runtime composer owns the UDP receiver backends and threads a
# caller-provided boost::asio::io_context through to the asio receiver,
# so it pulls in the asio network target. The pure domain code still
# only depends on kraken::kraken.
target_link_libraries(order_routing
  PUBLIC
    kraken::kraken
    kraken::network::asio
    kraken::network::ef_vi
)
