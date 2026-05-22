project(morfix_quickfix VERSION 0.1.0
  DESCRIPTION "Glue between morfix, ospec, and the local QuickFIX boundary"
  LANGUAGES CXX)

add_library(morfix_quickfix STATIC
  src/codecs.cpp
)
add_library(morfix_quickfix::morfix_quickfix ALIAS morfix_quickfix)

target_include_directories(morfix_quickfix
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(morfix_quickfix
  PUBLIC
    lab::core
    morfix::morfix
    ospec::ospec
    quickfix_fix::quickfix_fix
)
