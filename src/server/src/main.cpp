#include "server/application.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_entry/runtime/session_config.hpp"

#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/json.hpp"
#include "lab/log.hpp"
#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include "nlohmann/json.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

struct app_config
{
  server::config application;
};

void print_usage(std::ostream& stream)
{
  stream << "usage: server CONFIG_FILE\n";
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

void require_type(const nlohmann::json& json_object, const std::string& expected_type)
{
  const auto actual_type = read_type(json_object);
  if (actual_type != expected_type) {
    throw std::runtime_error{"expected config type '" + expected_type + "', got '" + actual_type + "'"};
  }
}

lab::network::types::endpoint_config read_endpoint(const nlohmann::json& json_object)
{
  lab::network::types::endpoint_config endpoint;
  lab::json::read_field(json_object, "address", endpoint.address);
  lab::json::read_field(json_object, "port", endpoint.port);
  return endpoint;
}

order_entry::runtime::receiver_config read_receiver_config(const nlohmann::json& json_object)
{
  const auto type = read_type(json_object);
  const auto endpoint = read_endpoint(json_object.at("endpoint"));

  if (type == "asio_udp") {
    return order_entry::runtime::asio_udp_receiver_config{.endpoint = endpoint};
  }
  if (type == "ef_vi_udp") {
    return order_entry::runtime::ef_vi_udp_receiver_config{.endpoint = endpoint};
  }

  throw std::runtime_error{"unknown order-entry receiver type '" + type + "'"};
}

order_entry::runtime::decoder_config read_decoder_config(const nlohmann::json& json_object)
{
  require_type(json_object, "json");
  return order_entry::runtime::json_decoder_config{};
}

order_entry::runtime::session_config read_order_entry_config(const nlohmann::json& json_object)
{
  return order_entry::runtime::session_config{
    .receiver = read_receiver_config(json_object.at("receiver")),
    .decoder = read_decoder_config(json_object.at("decoder")),
  };
}

matching_engine::runtime::engine_config read_matching_engine_config(const nlohmann::json& json_object)
{
  matching_engine::runtime::engine_config config;
  lab::json::read_field(json_object, "valid_symbols", config.valid_symbols);
  lab::json::read_field(json_object, "expected_resting_orders", config.expected_resting_orders);
  lab::json::read_field(json_object, "node_pool_chunk_size", config.node_pool_chunk_size);
  return config;
}

market_data::runtime::encoder_config read_market_data_encoder_config(const nlohmann::json& json_object)
{
  require_type(json_object, "json");
  return market_data::runtime::json_encoder_config{};
}

market_data::runtime::sink_config read_market_data_sink_config(const nlohmann::json& json_object)
{
  require_type(json_object, "spdlog");
  return market_data::runtime::spdlog_sink_config{};
}

market_data::runtime::publisher_config read_market_data_config(const nlohmann::json& json_object)
{
  return market_data::runtime::publisher_config{
    .encoder = read_market_data_encoder_config(json_object.at("encoder")),
    .sink = read_market_data_sink_config(json_object.at("sink")),
  };
}

lab::logger_config read_logger_config(const nlohmann::json& json_object)
{
  const auto type = read_type(json_object);
  if (type == "console") {
    return lab::console_logger_config{};
  }
  if (type == "null") {
    return lab::null_logger_config{};
  }
  if (type == "file") {
    std::string path;
    lab::json::read_field(json_object, "path", path);
    return lab::file_logger_config{.path = std::move(path)};
  }

  throw std::runtime_error{"unknown logger type '" + type + "'"};
}

lab::event_loop_idle_strategy read_idle_strategy(const nlohmann::json& json_object)
{
  const auto type = read_type(json_object);
  if (type == "busy_spin") {
    return lab::busy_spin_idle{};
  }
  if (type == "timed_wait") {
    std::chrono::microseconds::rep duration_microseconds = 0;
    lab::json::read_field(json_object, "duration_microseconds", duration_microseconds);
    return lab::timed_wait_idle{.duration = std::chrono::microseconds{duration_microseconds}};
  }

  throw std::runtime_error{"unknown event-loop idle strategy '" + type + "'"};
}

lab::event_loop_config read_event_loop_config(const nlohmann::json& json_object)
{
  lab::event_loop_config config;
  lab::json::read_field(json_object, "thread_name", config.thread_name);
  lab::json::read_field(json_object, "queue_capacity", config.queue_capacity);
  config.idle_strategy = read_idle_strategy(json_object.at("idle_strategy"));
  return config;
}

void from_json(const nlohmann::json& json_object, app_config& config)
{
  config.application = server::config{
    .order_entry = read_order_entry_config(json_object.at("order_entry")),
    .matching_engine = read_matching_engine_config(json_object.at("matching_engine")),
    .market_data = read_market_data_config(json_object.at("market_data")),
    .logger = read_logger_config(json_object.at("logger")),
    .input_thread = read_event_loop_config(json_object.at("threads").at("input")),
    .processing_thread = read_event_loop_config(json_object.at("threads").at("processing")),
    .output_thread = read_event_loop_config(json_object.at("threads").at("output")),
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
  const auto result = boost::leaf::try_handle_all(
    [&]() -> lab::result<int> {
      BOOST_LEAF_ASSIGN(const auto config_path, parse_config_path(argc, argv));
      if (config_path.empty()) {
        return EXIT_SUCCESS;
      }

      BOOST_LEAF_ASSIGN(const auto config, lab::json::read_from_file<app_config>(config_path, true));
      server::application app{config.application};

      std::signal(SIGINT, signal_handler);
      std::signal(SIGTERM, signal_handler);

      LAB_LEAF_CHECK(app.start());

      // TODO: replace the signal poll with a lifecycle notification primitive.
      while (!g_stop_requested.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
      }

      app.stop();
      return EXIT_SUCCESS;
    },
    LAB_RESULT_CATCH_ALL(print_usage(std::cerr); return EXIT_FAILURE;));

  return result;
}
