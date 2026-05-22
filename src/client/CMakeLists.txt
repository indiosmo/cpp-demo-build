project(client VERSION 0.1.0
  DESCRIPTION "Matching Engine Lab UDP client application"
  LANGUAGES CXX)

add_executable(client
  src/main.cpp
)

target_link_libraries(client
  PRIVATE
    order_client::order_client
)

set_target_properties(client PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
