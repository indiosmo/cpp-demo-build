project(mmdfix VERSION 0.1.0
  DESCRIPTION "Canonical FIX-shaped market-data messages"
  LANGUAGES CXX)

add_library(mmdfix STATIC
  src/conversions.cpp
)
add_library(mmdfix::mmdfix ALIAS mmdfix)

target_include_directories(mmdfix
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mmdfix
  PUBLIC
    lab::core
    mmd::mmd
)
