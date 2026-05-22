project(matching_engine_lab_server VERSION 0.1.0
  DESCRIPTION "Matching Engine Lab UDP server application"
  LANGUAGES CXX)

find_package(Threads REQUIRED)

# Application library: owns the pipeline composition and is the
# unit-testable surface. The executable below is a thin wrapper that
# wires signal handling around it.
add_library(matching_engine_lab_server_app STATIC
  src/application.cpp
)
add_library(matching_engine_lab_server::app ALIAS matching_engine_lab_server_app)

target_include_directories(matching_engine_lab_server_app
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(matching_engine_lab_server_app
  PUBLIC
    lab::core
    order_routing::order_routing
    matching_engine::matching_engine
    market_data::market_data
    Threads::Threads
)

# Thin executable per ADR 0001: just hooks the signal handlers around the
# application's start/run/stop lifecycle.
add_executable(matching_engine_lab_server
  src/main.cpp
)

target_link_libraries(matching_engine_lab_server
  PRIVATE
    matching_engine_lab_server::app
)

# Place the binary at <build>/matching_engine_lab_server so direct run commands do not
# depend on target-specific subdirectories.
set_target_properties(matching_engine_lab_server PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
