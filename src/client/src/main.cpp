#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/json.hpp"
#include "lab/network/types.hpp"
#include "lab/result.hpp"
#include "order_client/client.hpp"
#include "order_entry/json_decoder.hpp"
#include "order_entry/messages.hpp"

#include "nlohmann/json.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

struct app_config
{
  order_client::config order_client;
  std::optional<std::string> input_path;
};

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

std::string read_type(const nlohmann::json& json_object)
{
  std::string type;
  lab::json::read_field(json_object, "type", type);
  return type;
}

lab::network::types::endpoint_config read_endpoint(const nlohmann::json& json_object)
{
  lab::network::types::endpoint_config endpoint;
  lab::json::read_field(json_object, "address", endpoint.address);
  lab::json::read_field(json_object, "port", endpoint.port);
  return endpoint;
}

order_client::config read_order_client_config(const nlohmann::json& json_object)
{
  return order_client::config{.endpoint = read_endpoint(json_object.at("endpoint"))};
}

std::optional<std::string> read_input_path(const nlohmann::json& json_object)
{
  const auto type = read_type(json_object);
  if (type == "stdin") {
    return std::nullopt;
  }
  if (type == "file") {
    std::string path;
    lab::json::read_field(json_object, "path", path);
    return path;
  }

  throw std::runtime_error{"unknown client input type '" + type + "'"};
}

void from_json(const nlohmann::json& json_object, app_config& config)
{
  config.order_client = read_order_client_config(json_object.at("order_client"));
  config.input_path = read_input_path(json_object.at("input"));
}

lab::result<void> send_commands(std::istream& input, order_client::client& client)
{
  order_entry::json_decoder decoder;

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

  if (config.input_path) {
    std::ifstream input{*config.input_path};
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
