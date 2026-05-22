#include "kraken_submission/application.hpp"

#include "market_data/messages.hpp"
#include "order_routing/messages.hpp"

#include "kraken/error_code.hpp"
#include "kraken/log.hpp"
#include "kraken/result.hpp"
#include "kraken/variant.hpp"

#include <utility>

namespace kraken_submission {

application::application(config configuration)
  : config_{std::move(configuration)}
  , input_loop_{config_.input_thread}
  , processing_loop_{config_.processing_thread}
  , output_loop_{config_.output_thread}
{
}

application::~application()
{
  stop();
}

void application::configure_logger()
{
  kraken::install_logger(config_.logger);
}

void application::wire_pipeline()
{
  // input loop -> processing loop hop.
  order_routing_.on_request = [this](const order_routing::request& req) {
    processing_loop_.post([this, req] { engine_.send(req); });
  };

  order_routing_.on_rejected = [](const order_routing::rejection& rej) {
    KRAKEN_LOG_WARN("rejected input: {} | {}", rej.raw_payload, rej.reason);
  };

  // processing loop -> output loop hop.
  engine_.on_event = [this](const market_data::message& ev) { output_loop_.post([this, ev] { publisher_.send(ev); }); };

  input_loop_.add_poller([this] { return order_routing_.poll(); });
}

kraken::result<void> application::start()
{
  if (started_) {
    return kraken::make_leaf_error(kraken::error_code::already_in_progress, "application already started");
  }

  configure_logger();

  // Only one of the receiver backends needs the shared io_context.
  kraken::match(
    config_.order_routing.receiver,
    [this](const order_routing::runtime::asio_udp_receiver_config&) { io_context_.emplace(); },
    [](const order_routing::runtime::ef_vi_udp_receiver_config&) {});

  KRAKEN_LEAF_CHECK(order_routing_.setup(config_.order_routing, io_context_ ? &*io_context_ : nullptr));
  KRAKEN_LEAF_CHECK(engine_.setup(config_.matching_engine));
  KRAKEN_LEAF_CHECK(publisher_.setup(config_.market_data));

  wire_pipeline();

  // Bind the socket before any thread starts so failures surface before
  // stdout sees any record.
  KRAKEN_LEAF_CHECK(order_routing_.start());

  // Outbound-to-inbound: each consumer is live before its producer can post.
  KRAKEN_LEAF_CHECK(output_loop_.start());
  KRAKEN_LEAF_CHECK(processing_loop_.start());
  KRAKEN_LEAF_CHECK(input_loop_.start());

  started_ = true;
  KRAKEN_LOG_INFO("application started");
  return {};
}

void application::run()
{
  input_loop_.join();
  processing_loop_.join();
  output_loop_.join();
  started_ = false;
}

void application::stop()
{
  if (!started_) {
    return;
  }

  KRAKEN_LOG_INFO("application stopping");

  order_routing_.stop();
  input_loop_.stop();
  input_loop_.join();

  processing_loop_.stop();
  processing_loop_.join();

  output_loop_.stop();
  output_loop_.join();

  started_ = false;

  KRAKEN_LOG_INFO("application stopped");
}

} // namespace kraken_submission
