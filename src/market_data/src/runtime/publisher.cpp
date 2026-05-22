#include "market_data/runtime/publisher.hpp"

#include "market_data/csv_encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "market_data/spdlog_sink.hpp"

#include "lab/error_code.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"
#include "lab/variant.hpp"

#include <memory>

namespace market_data::runtime {

publisher::~publisher() = default;

lab::result<void> publisher::setup(const publisher_config& config)
{
  if (publisher_.has_value()) {
    return lab::make_leaf_error(lab::error_code::already_in_progress, "market_data runtime publisher already set up");
  }

  lab::match(config.encoder, [this](const auto& encoder_cfg) { setup_encoder(encoder_cfg); });
  lab::match(config.sink, [this](const auto& sink_cfg) { setup_sink(sink_cfg); });

  publisher_.emplace(*encoder_, *sink_);

  LAB_LOG_INFO("market_data runtime publisher set up");
  return {};
}

void publisher::send(const market_data::message& msg)
{
  publisher_->send(msg);
}

void publisher::setup_encoder(const csv_encoder_config&)
{
  encoder_ = std::make_unique<csv_encoder>();
}

void publisher::setup_sink(const spdlog_sink_config&)
{
  sink_ = std::make_unique<spdlog_sink>();
}

} // namespace market_data::runtime
