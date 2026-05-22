#include "matching_engine/runtime/engine.hpp"

#include "market_data/messages.hpp"
#include "matching_engine/engine.hpp"
#include "matching_engine/engine_config.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_entry/messages.hpp"

#include "lab/error_code.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"

#include <utility>

namespace matching_engine::runtime {

engine::~engine() = default;

lab::result<void> engine::setup(engine_config config)
{
  if (impl_.has_value()) {
    return lab::make_leaf_error(lab::error_code::already_in_progress, "matching_engine runtime engine already set up");
  }

  auto& inner = impl_.emplace(
    matching_engine::engine_config{
      .valid_symbols = std::move(config.valid_symbols),
      .expected_resting_orders = config.expected_resting_orders,
      .node_pool_chunk_size = config.node_pool_chunk_size,
    });

  inner.on_market_data = [this](const market_data::message& ev) { on_market_data(ev); };
  inner.on_order_entry = [this](const order_entry::event& ev) { on_order_entry(ev); };

  LAB_LOG_INFO("matching_engine runtime engine set up");
  return {};
}

void engine::send(const order_entry::request& req)
{
  impl_->send(req);
}

} // namespace matching_engine::runtime
