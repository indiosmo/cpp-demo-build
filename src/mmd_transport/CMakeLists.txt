project(mmd_transport VERSION 0.1.0
  DESCRIPTION "Market-data delivery transport boundaries"
  LANGUAGES CXX)

add_library(mmd_transport STATIC
  src/sinks.cpp
)
add_library(mmd_transport::mmd_transport ALIAS mmd_transport)

target_include_directories(mmd_transport
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mmd_transport
  PUBLIC
    lab::core
)
