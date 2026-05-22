#include "matching_engine/runtime/engine.hpp"

#include "market_data/messages.hpp"
#include "matching_engine/engine.hpp"
#include "matching_engine/engine_config.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_routing/messages.hpp"

#include "kraken/error_code.hpp"
#include "kraken/log.hpp"
#include "kraken/result.hpp"

#include <utility>

namespace matching_engine::runtime {

engine::~engine() = default;

kraken::result<void> engine::setup(engine_config config)
{
  if (impl_.has_value()) {
    return kraken::make_leaf_error(kraken::error_code::already_in_progress, "matching_engine runtime engine already set up");
  }

  auto& inner = impl_.emplace(
    matching_engine::engine_config{
      .valid_symbols = std::move(config.valid_symbols),
      .expected_resting_orders = config.expected_resting_orders,
      .node_pool_chunk_size = config.node_pool_chunk_size,
    });

  inner.on_event = [this](const market_data::message& ev) { on_event(ev); };

  KRAKEN_LOG_INFO("matching_engine runtime engine set up");
  return {};
}

void engine::send(const order_routing::request& req)
{
  impl_->send(req);
}

} // namespace matching_engine::runtime
