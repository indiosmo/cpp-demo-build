project(mmd_json VERSION 0.1.0
  DESCRIPTION "JSON rendering for normalized market-data events"
  LANGUAGES CXX)

add_library(mmd_json STATIC
  src/json_encoder.cpp
)
add_library(mmd_json::mmd_json ALIAS mmd_json)

target_include_directories(mmd_json
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mmd_json
  PUBLIC
    lab::core
    mmd::mmd
)
