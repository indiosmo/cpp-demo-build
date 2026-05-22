#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/json.hpp"
#include "lab/result.hpp"
#include "order_client/client.hpp"
#include "order_entry/json_decoder.hpp"
#include "order_entry/messages.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

struct stdin_input_config
{
};

LAB_AUTO_JSON(stdin_input_config)

struct file_input_config
{
  std::string path;
};

LAB_AUTO_JSON(file_input_config, path)

using input_config = std::variant<stdin_input_config, file_input_config>;

void to_json(nlohmann::json& json_object, const input_config& config)
{
  std::visit(
    [&](const auto& selection) {
      using selection_t = std::decay_t<decltype(selection)>;
      json_object = selection;
      if constexpr (std::is_same_v<selection_t, stdin_input_config>) {
        json_object["type"] = "stdin";
      } else if constexpr (std::is_same_v<selection_t, file_input_config>) {
        json_object["type"] = "file";
      }
    },
    config);
}

void from_json(const nlohmann::json& json_object, input_config& config)
{
  const auto type = lab::json::read_type(json_object);
  if (type == "stdin") {
    config = stdin_input_config{};
    return;
  }
  if (type == "file") {
    file_input_config file;
    lab::json::read_field(json_object, "path", file.path);
    config = std::move(file);
    return;
  }

  throw std::runtime_error{"unknown client input type '" + type + "'"};
}

struct app_config
{
  order_client::client_config order_client;
  input_config input;
};

LAB_AUTO_JSON(app_config, order_client, input)

void print_usage(std::ostream& stream)
{
  stream << "usage: client CONFIG_FILE\n";
}

lab::result<std::string> parse_config_path(int argc, char** argv)
{
  if (argc == 2) {
    const std::string argument{argv[1]};
    if (argument == "--help" || argument == "-h") {
      print_usage(std::cout);
      return {};
    }
    return argument;
  }

  return lab::make_leaf_error(lab::error_code::configuration_error, "expected one config file path");
}

std::optional<std::string> input_path(const input_config& config)
{
  return std::visit(
    [](const auto& selection) -> std::optional<std::string> {
      using selection_t = std::decay_t<decltype(selection)>;
      if constexpr (std::is_same_v<selection_t, file_input_config>) {
        return selection.path;
      } else {
        return std::nullopt;
      }
    },
    config);
}

lab::result<void> send_commands(std::istream& input, order_client::client& client)
{
  order_entry::json_decoder decoder{
    order_entry::json_decoder_config{
      .max_datagram_size = 65535,
    }};

  for (std::string line; std::getline(input, line);) {
    if (line.empty()) {
      continue;
    }

    BOOST_LEAF_ASSIGN(const auto request, decoder.decode(line));
    LAB_LEAF_CHECK(client.send(request));
  }

  return {};
}

lab::result<void> run_client(const app_config& config)
{
  order_client::client client{config.order_client};

  LAB_LEAF_CHECK(client.connect());

  const auto configured_input_path = input_path(config.input);
  if (configured_input_path) {
    std::ifstream input{*configured_input_path};
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
      BOOST_LEAF_ASSIGN(const auto config_path, parse_config_path(argc, argv));
      if (config_path.empty()) {
        return EXIT_SUCCESS;
      }

      BOOST_LEAF_ASSIGN(const auto config, lab::json::read_from_file<app_config>(config_path, true));
      LAB_LEAF_CHECK(run_client(config));
      return EXIT_SUCCESS;
    },
    LAB_RESULT_CATCH_ALL(print_usage(std::cerr); return EXIT_FAILURE;));

  return result;
}
