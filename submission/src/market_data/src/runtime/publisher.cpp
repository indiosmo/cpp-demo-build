#include "market_data/runtime/publisher.hpp"

#include "market_data/csv_encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "market_data/spdlog_sink.hpp"

#include "kraken/error_code.hpp"
#include "kraken/log.hpp"
#include "kraken/result.hpp"
#include "kraken/variant.hpp"

#include <memory>

namespace market_data::runtime {

publisher::~publisher() = default;

kraken::result<void> publisher::setup(const publisher_config& config)
{
  if (publisher_.has_value()) {
    return kraken::make_leaf_error(kraken::error_code::already_in_progress, "market_data runtime publisher already set up");
  }

  kraken::match(config.encoder, [this](const auto& encoder_cfg) { setup_encoder(encoder_cfg); });
  kraken::match(config.sink, [this](const auto& sink_cfg) { setup_sink(sink_cfg); });

  publisher_.emplace(*encoder_, *sink_);

  KRAKEN_LOG_INFO("market_data runtime publisher set up");
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
