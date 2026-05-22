project(morfix VERSION 0.1.0
  DESCRIPTION "Canonical FIX-shaped order-routing messages"
  LANGUAGES CXX)

add_library(morfix STATIC
  src/conversions.cpp
  src/session.cpp
)
add_library(morfix::morfix ALIAS morfix)

target_include_directories(morfix
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(morfix
  PUBLIC
    lab::core
    mor::mor
)
