add_library(matching_engine STATIC
  src/runtime/engine.cpp
  src/v3/engine.cpp
)
add_library(matching_engine::matching_engine ALIAS matching_engine)

target_include_directories(matching_engine
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(matching_engine
  PUBLIC
    lab::core
    order_entry::order_entry
    market_data::market_data
)
