project(order_client VERSION 0.1.0
  DESCRIPTION "Matching Engine Lab order client library"
  LANGUAGES CXX)

find_package(Boost REQUIRED)

add_library(order_client STATIC
  src/client.cpp
  src/csv_encoder.cpp
  src/udp_sender.cpp
)
add_library(order_client::order_client ALIAS order_client)

target_include_directories(order_client
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(order_client
  PUBLIC
    lab::core
    lab::network::asio
    order_routing::order_routing
    Boost::headers
)

set_source_files_properties(
  src/udp_sender.cpp
  PROPERTIES COMPILE_OPTIONS "-Wno-null-dereference"
)
