#ifndef MARKET_DATA_RUNTIME_PUBLISHER_CONFIG_HPP
#define MARKET_DATA_RUNTIME_PUBLISHER_CONFIG_HPP

#include "lab/json.hpp"

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

/*
 * Configuration surface for market_data::runtime::publisher. A new encoder
 * or sink backend is added as a variant alternative below plus a matching
 * setup overload in the runtime.
 */

namespace market_data::runtime {

struct json_encoder_config
{
};

LAB_AUTO_JSON(json_encoder_config)

using encoder_config = std::variant<json_encoder_config>;

struct spdlog_sink_config
{
};

LAB_AUTO_JSON(spdlog_sink_config)

using sink_config = std::variant<spdlog_sink_config>;

struct publisher_config
{
  encoder_config encoder{json_encoder_config{}};
  sink_config sink{spdlog_sink_config{}};
};

BOOST_DESCRIBE_STRUCT(publisher_config, (), (encoder, sink))

inline void to_json(nlohmann::json& json_object, const encoder_config& config)
{
  std::visit(
    [&](const auto& selection) {
      json_object = selection;
      json_object["type"] = "json";
    },
    config);
}

inline void from_json(const nlohmann::json& json_object, encoder_config& config)
{
  const auto type = lab::json::read_type(json_object);
  if (type == "json") {
    config = json_encoder_config{};
    return;
  }

  throw std::runtime_error{"unknown market-data encoder type '" + type + "'"};
}

inline void to_json(nlohmann::json& json_object, const sink_config& config)
{
  std::visit(
    [&](const auto& selection) {
      json_object = selection;
      json_object["type"] = "spdlog";
    },
    config);
}

inline void from_json(const nlohmann::json& json_object, sink_config& config)
{
  const auto type = lab::json::read_type(json_object);
  if (type == "spdlog") {
    config = spdlog_sink_config{};
    return;
  }

  throw std::runtime_error{"unknown market-data sink type '" + type + "'"};
}

inline void to_json(nlohmann::json& json_object, const publisher_config& config)
{
  lab::json::write_described_object(json_object, config);
}

inline void from_json(const nlohmann::json& json_object, publisher_config& config)
{
  if (json_object.contains("encoder")) {
    lab::json::read_field(json_object, "encoder", config.encoder);
  }
  if (json_object.contains("sink")) {
    lab::json::read_field(json_object, "sink", config.sink);
  }
}

} // namespace market_data::runtime

#endif /* MARKET_DATA_RUNTIME_PUBLISHER_CONFIG_HPP */
