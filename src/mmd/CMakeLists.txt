project(mmd VERSION 0.1.0
  DESCRIPTION "Normalized matching-engine market-data domain"
  LANGUAGES CXX)

add_library(mmd STATIC
  src/conversions.cpp
)
add_library(mmd::mmd ALIAS mmd)

target_include_directories(mmd
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mmd
  PUBLIC
    lab::core
    market_data::market_data
)
