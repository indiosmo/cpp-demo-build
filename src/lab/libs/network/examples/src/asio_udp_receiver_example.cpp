/*
 * Standalone example for lab::network::asio_udp_receiver. Binds a UDP
 * socket, drives an io_context on a dedicated thread, prints one line per
 * datagram, and exits on enter. Stays at the library boundary -- construct
 * with an injected io_context, wire on_datagram, start, drive, stop -- so a
 * reader can see how to drop the receiver into their own runtime.
 */

#include "boost/asio/io_context.hpp"
#include "boost/leaf/handle_errors.hpp"

#include "lab/error.hpp"
#include "lab/fmt.hpp"
#include "lab/network/asio_udp_receiver.hpp"
#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <string_view>
#include <system_error>
#include <thread>

namespace kn = lab::network;

namespace {

struct example_config
{
  std::string address;
  std::uint16_t port;
};

lab::result<example_config> parse_args(int argc, char** argv)
{
  example_config config{
    .address = "127.0.0.1",
    .port = 9001,
  };

  if (argc >= 2) {
    config.address = argv[1];
  }

  if (argc >= 3) {
    const std::string_view port_argument{argv[2]};
    std::uint16_t parsed_port{};
    const auto* port_end = port_argument.data() + port_argument.size();
    const auto parse_result = std::from_chars(port_argument.data(), port_end, parsed_port);
    if (parse_result.ec != std::errc{} || parse_result.ptr != port_end) {
      return lab::make_leaf_error(lab::error_code::configuration_error, fmt::format("invalid port '{}'", port_argument));
    }
    config.port = parsed_port;
  }

  return config;
}

void print_datagram(std::uint64_t sequence, kn::types::datagram_view payload)
{
  // First 32 bytes as hex so the reader can eyeball the socket round-trip.
  constexpr std::size_t preview_byte_count = 32;
  const auto preview = payload.substr(0, std::min(payload.size(), preview_byte_count));

  fmt::memory_buffer hex;
  for (char byte : preview) {
    fmt::format_to(std::back_inserter(hex), "{:02x} ", static_cast<unsigned>(static_cast<unsigned char>(byte)));
  }

  fmt::print(
    "datagram {:>6}: {:>5} bytes  {}{}\n",
    sequence,
    payload.size(),
    std::string_view{hex.data(), hex.size()},
    payload.size() > preview_byte_count ? "..." : "");
  std::fflush(stdout);
}

} // namespace

int main(int argc, char** argv)
{
  return boost::leaf::try_handle_all(
    [&]() -> lab::result<int> {
      BOOST_LEAF_ASSIGN(const auto config, parse_args(argc, argv));

      boost::asio::io_context io_context;
      kn::asio_udp_receiver receiver{
        io_context,
        kn::types::endpoint_config{
          .address = config.address,
          .port = config.port,
        }};

      std::atomic<std::uint64_t> received_count{0};
      receiver.on_datagram = [&](kn::types::datagram_view bytes) {
        const auto sequence = received_count.fetch_add(1, std::memory_order_relaxed) + 1;
        print_datagram(sequence, bytes);
      };

      LAB_LEAF_CHECK(receiver.start());

      // io_context runs on its own thread so main can wait on stdin and
      // trigger a clean shutdown by cancelling the socket.
      std::thread receive_thread{[&] { io_context.run(); }};

      fmt::print("listening on {}:{}\n", config.address, config.port);
      fmt::print("send datagrams with: echo -n hello | nc -u -w1 {} {}\n", config.address, config.port);
      fmt::print("press enter to stop\n");
      std::fflush(stdout);

      std::getchar();

      receiver.stop();
      receive_thread.join();

      fmt::print("received {} datagram(s)\n", received_count.load(std::memory_order_relaxed));
      return 0;
    },
    LAB_RESULT_CATCH_ALL(return EXIT_FAILURE;));
}
