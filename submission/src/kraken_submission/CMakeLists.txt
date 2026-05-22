project(kraken_submission VERSION 0.1.0
  DESCRIPTION "Kraken senior order book submission application"
  LANGUAGES CXX)

find_package(Threads REQUIRED)

# Application library: owns the pipeline composition and is the
# unit-testable surface. The executable below is a thin wrapper that
# wires signal handling around it.
add_library(kraken_submission_app STATIC
  src/application.cpp
)
add_library(kraken_submission::app ALIAS kraken_submission_app)

target_include_directories(kraken_submission_app
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(kraken_submission_app
  PUBLIC
    kraken::kraken
    order_routing::order_routing
    matching_engine::matching_engine
    market_data::market_data
    Threads::Threads
)

# Thin executable per ADR 0001: just hooks the signal handlers around the
# application's start/run/stop lifecycle.
add_executable(kraken_submission
  src/main.cpp
)

target_link_libraries(kraken_submission
  PRIVATE
    kraken_submission::app
)

# Place the binary at <build>/kraken_submission so the docker test runner can
# locate it at /kraken_submission/build/kraken_submission as it does today.
set_target_properties(kraken_submission PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
