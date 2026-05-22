project(server VERSION 0.1.0
  DESCRIPTION "Matching Engine Lab UDP server application"
  LANGUAGES CXX)

find_package(Threads REQUIRED)

# Application library: owns the pipeline composition and is the
# unit-testable surface. The executable below is a thin wrapper that
# wires signal handling around it.
add_library(server_app STATIC
  src/application.cpp
)
add_library(server::app ALIAS server_app)

target_include_directories(server_app
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(server_app
  PUBLIC
    lab::core
    order_routing::order_routing
    matching_engine::matching_engine
    market_data::market_data
    Threads::Threads
)

# Thin executable per ADR 0001: just hooks the signal handlers around the
# application's start/run/stop lifecycle.
add_executable(server
  src/main.cpp
)

target_link_libraries(server
  PRIVATE
    server::app
)

# Place the binary at <build>/server for direct run commands.
set_target_properties(server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
