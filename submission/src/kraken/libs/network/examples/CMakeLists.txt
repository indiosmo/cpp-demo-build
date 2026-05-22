add_executable(asio_udp_receiver_example
  src/asio_udp_receiver_example.cpp
)

target_link_libraries(asio_udp_receiver_example
  PRIVATE
    kraken::network::asio
    kraken::kraken
)
