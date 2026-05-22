project(quickfix_fix VERSION 0.1.0
  DESCRIPTION "Local QuickFIX-compatible FIX engine boundary"
  LANGUAGES CXX)

add_library(quickfix_fix STATIC
  src/codec.cpp
  src/session.cpp
)
add_library(quickfix_fix::quickfix_fix ALIAS quickfix_fix)

target_include_directories(quickfix_fix
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(quickfix_fix
  PUBLIC
    lab::core
)
