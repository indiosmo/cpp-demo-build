#ifndef MARKET_DATA_RUNTIME_PUBLISHER_CONFIG_HPP
#define MARKET_DATA_RUNTIME_PUBLISHER_CONFIG_HPP

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

using encoder_config = std::variant<json_encoder_config>;

struct spdlog_sink_config
{
};

using sink_config = std::variant<spdlog_sink_config>;

struct publisher_config
{
  encoder_config encoder{json_encoder_config{}};
  sink_config sink{spdlog_sink_config{}};
};

} // namespace market_data::runtime

#endif /* MARKET_DATA_RUNTIME_PUBLISHER_CONFIG_HPP */
