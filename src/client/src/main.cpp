#include "order_client/client.hpp"
#include "order_routing/csv_decoder.hpp"
#include "order_routing/messages.hpp"

#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/fmt.hpp"
#include "lab/result.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

struct cli_options
{
  std::string host = "127.0.0.1";
  std::uint16_t port = 1234;
  std::optional<std::string> input_path;
  bool show_help = false;
};

void print_usage(std::ostream& stream)
{
  stream << "usage: client [--host ADDRESS] [--port PORT] [--input FILE]\n";
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
    } else if (argument == "--input") {
      BOOST_LEAF_ASSIGN(const auto value, require_value(argument));
      options.input_path = std::string{value};
    } else {
      return lab::make_leaf_error(lab::error_code::configuration_error, fmt::format("unknown option {}", argument));
    }
  }

  return options;
}

lab::result<void> send_commands(std::istream& input, order_client::client& client)
{
  order_routing::csv_decoder decoder;

  for (std::string line; std::getline(input, line);) {
    if (line.empty()) {
      continue;
    }

    BOOST_LEAF_ASSIGN(const auto request, decoder.decode(line));
    LAB_LEAF_CHECK(client.send(request));
  }

  return {};
}

lab::result<void> run_client(const cli_options& options)
{
  order_client::client client{order_client::config{
    .endpoint{
      .address = options.host,
      .port = options.port,
    },
  }};

  LAB_LEAF_CHECK(client.connect());

  if (options.input_path) {
    std::ifstream input{*options.input_path};
    if (!input) {
      return lab::make_leaf_error(lab::error_code::configuration_error, "could not open input file");
    }
    return send_commands(input, client);
  }

  return send_commands(std::cin, client);
}

} // namespace

int main(int argc, char** argv)
{
  const auto result = boost::leaf::try_handle_all(
    [&]() -> lab::result<int> {
      BOOST_LEAF_ASSIGN(const auto options, parse_arguments(argc, argv));

      if (options.show_help) {
        print_usage(std::cout);
        return EXIT_SUCCESS;
      }

      LAB_LEAF_CHECK(run_client(options));
      return EXIT_SUCCESS;
    },
    LAB_RESULT_CATCH_ALL(
      print_usage(std::cerr);
      return EXIT_FAILURE;));

  return result;
}
