project(mor VERSION 0.1.0
  DESCRIPTION "Normalized matching-engine order routing domain"
  LANGUAGES CXX)

add_library(mor STATIC
  src/conversions.cpp
)
add_library(mor::mor ALIAS mor)

target_include_directories(mor
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mor
  PUBLIC
    lab::core
    order_entry::order_entry
)
