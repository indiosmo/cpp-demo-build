project(ospec VERSION 0.1.0
  DESCRIPTION "Venue-specific order-routing and market-data specifications"
  LANGUAGES CXX)

add_library(ospec STATIC
  src/b3.cpp
)
add_library(ospec::ospec ALIAS ospec)

target_include_directories(ospec
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(ospec
  PUBLIC
    lab::core
    mor::mor
    mmd::mmd
)
