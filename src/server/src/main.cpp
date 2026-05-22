#include "server/application.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_routing/runtime/session_config.hpp"
#include "order_routing/types.hpp"

#include "lab/fmt.hpp"
#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace {

// The sample UDP protocol uses port 1234 by default.
constexpr std::uint16_t default_udp_port = 1234;
constexpr const char* default_listen_address = "0.0.0.0";

struct cli_options
{
  std::string host = default_listen_address;
  std::uint16_t port = default_udp_port;
  bool show_help = false;
};

void print_usage(std::ostream& stream)
{
  stream << "usage: server [--host ADDRESS] [--port PORT]\n";
}

lab::result<std::uint16_t> parse_port(std::string_view text)
{
  unsigned int value = 0;
  const auto* first = text.data();
  const auto* last = text.data() + text.size();
  const auto [ptr, error] = std::from_chars(first, last, value);
  if (error != std::errc{} || ptr != last || value > 65535) {
    return lab::make_leaf_error(lab::error_code::configuration_error, "invalid UDP port");
  }
  return static_cast<std::uint16_t>(value);
}

lab::result<cli_options> parse_arguments(int argc, char** argv)
{
  cli_options options;

  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};

    if (argument == "--help" || argument == "-h") {
      options.show_help = true;
      return options;
    }

    auto require_value = [&](std::string_view option) -> lab::result<std::string_view> {
      if (index + 1 >= argc) {
        return lab::make_leaf_error(lab::error_code::configuration_error, fmt::format("{} requires a value", option));
      }
      ++index;
      return std::string_view{argv[index]};
    };

    if (argument == "--host") {
      BOOST_LEAF_ASSIGN(const auto value, require_value(argument));
      options.host = value;
    } else if (argument == "--port") {
      BOOST_LEAF_ASSIGN(const auto value, require_value(argument));
      BOOST_LEAF_ASSIGN(options.port, parse_port(value));
    } else {
      return lab::make_leaf_error(lab::error_code::configuration_error, fmt::format("unknown option {}", argument));
    }
  }

  return options;
}

// Pre-allocated symbol set keeps local demos deterministic; other symbols are
// rejected at the engine boundary. A real deployment would source this list
// from an exchange-membership feed at startup.
matching_engine::runtime::engine_config make_engine_config()
{
  return matching_engine::runtime::engine_config{
    .valid_symbols{
      order_routing::types::symbol{"IBM"},
      order_routing::types::symbol{"AAPL"},
      order_routing::types::symbol{"VAL"},
    },
  };
}

std::atomic<bool> g_stop_requested = false;

void signal_handler(int /* signal */)
{
  g_stop_requested.store(true, std::memory_order_release);
}

} // namespace

int main(int argc, char** argv)
{
  bool parse_failed = false;
  const auto options_result = boost::leaf::try_handle_all(
    [&]() -> lab::result<cli_options> { return parse_arguments(argc, argv); },
    LAB_RESULT_CATCH_ALL(
      print_usage(std::cerr);
      parse_failed = true;
      return cli_options{};));

  if (parse_failed) {
    return EXIT_FAILURE;
  }

  if (options_result.show_help) {
    print_usage(std::cout);
    return EXIT_SUCCESS;
  }

  server::application app{server::config{
    .order_routing{
      .receiver{order_routing::runtime::asio_udp_receiver_config{
        .endpoint{
          .address = options_result.host,
          .port = options_result.port,
        },
      }},
    },
    .matching_engine = make_engine_config(),
    .market_data{},
  }};

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  LAB_EXIT_ON_ERROR(app.start());

  // IMPROVEMENT: replace the poll with a condition variable signalled when
  // every loop has stopped and drained.
  while (!g_stop_requested.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  app.stop();
  return EXIT_SUCCESS;
}
