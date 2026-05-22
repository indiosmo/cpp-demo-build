add_library(market_data STATIC
  src/json_encoder.cpp
  src/publisher.cpp
  src/spdlog_sink.cpp
  src/runtime/publisher.cpp
)
add_library(market_data::market_data ALIAS market_data)

target_include_directories(market_data
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# spdlog backs spdlog_sink. It is a PRIVATE link: only the .cpp pulls in
# <spdlog/...>; the header forward-declares spdlog::logger, so downstream
# targets do not need spdlog on their include path.
target_link_libraries(market_data
  PUBLIC
    lab::core
  PRIVATE
    spdlog::spdlog
)
