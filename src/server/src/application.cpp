#include "server/application.hpp"

#include "market_data/messages.hpp"
#include "order_entry/messages.hpp"

#include "lab/error_code.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"
#include "lab/variant.hpp"

#include <utility>

namespace server {

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
  lab::install_logger(config_.logger);
}

void application::wire_pipeline()
{
  // input loop -> processing loop hop.
  order_entry_.on_request = [this](const order_entry::request& req) {
    processing_loop_.post([this, req] { engine_.send(req); });
  };

  order_entry_.on_rejected = [](const order_entry::rejection& rej) {
    LAB_LOG_WARN("rejected input: {} | {}", rej.raw_payload, rej.reason);
  };

  // processing loop -> output loop hop.
  engine_.on_market_data = [this](const market_data::message& ev) { output_loop_.post([this, ev] { publisher_.send(ev); }); };
  engine_.on_order_entry = [](const order_entry::event&) {};

  input_loop_.add_poller([this] { return order_entry_.poll(); });
}

lab::result<void> application::start()
{
  if (started_) {
    return lab::make_leaf_error(lab::error_code::already_in_progress, "application already started");
  }

  configure_logger();

  // Only one of the receiver backends needs the shared io_context.
  lab::match(
    config_.order_entry.receiver,
    [this](const order_entry::runtime::asio_udp_receiver_config&) { io_context_.emplace(); },
    [](const order_entry::runtime::ef_vi_udp_receiver_config&) {});

  LAB_LEAF_CHECK(order_entry_.setup(config_.order_entry, io_context_ ? &*io_context_ : nullptr));
  LAB_LEAF_CHECK(engine_.setup(config_.matching_engine));
  LAB_LEAF_CHECK(publisher_.setup(config_.market_data));

  wire_pipeline();

  // Bind the socket before any thread starts so failures surface before
  // stdout sees any record.
  LAB_LEAF_CHECK(order_entry_.start());

  // Outbound-to-inbound: each consumer is live before its producer can post.
  LAB_LEAF_CHECK(output_loop_.start());
  LAB_LEAF_CHECK(processing_loop_.start());
  LAB_LEAF_CHECK(input_loop_.start());

  started_ = true;
  LAB_LOG_INFO("application started");
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

  LAB_LOG_INFO("application stopping");

  order_entry_.stop();
  input_loop_.stop();
  input_loop_.join();

  processing_loop_.stop();
  processing_loop_.join();

  output_loop_.stop();
  output_loop_.join();

  started_ = false;

  LAB_LOG_INFO("application stopped");
}

} // namespace server
