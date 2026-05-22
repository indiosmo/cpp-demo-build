#include "order_routing/runtime/session.hpp"

#include "boost/leaf/handle_errors.hpp"
#include "order_routing/csv_decoder.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/runtime/session_config.hpp"

#include "kraken/error_code.hpp"
#include "kraken/log.hpp"
#include "kraken/network/asio_udp_receiver.hpp"
#include "kraken/network/ef_vi_udp_receiver.hpp"
#include "kraken/network/types.hpp"
#include "kraken/result.hpp"
#include "kraken/variant.hpp"

#include <memory>
#include <string_view>
#include <utility>

namespace order_routing::runtime {

session::~session() = default;

kraken::result<void> session::setup(const session_config& config, boost::asio::io_context* io_context)
{
  if (session_.has_value()) {
    return kraken::make_leaf_error(kraken::error_code::already_in_progress, "order_routing runtime session already set up");
  }

  // Order matters: decoder, then inner session (captures *decoder_), then
  // receiver (forwards datagrams into session_->send).
  kraken::match(config.decoder, [this](const auto& decoder_cfg) { setup_decoder(decoder_cfg); });

  session_.emplace(*decoder_);

  session_->on_request = [this](const order_routing::request& req) { on_request(req); };
  session_->on_rejected = [this](const order_routing::rejection& rej) { on_rejected(rej); };

  KRAKEN_LEAF_CHECK(
    kraken::match(
      config.receiver,
      [this, io_context](const asio_udp_receiver_config& receiver_cfg) -> kraken::result<void> {
        if (io_context == nullptr) {
          return kraken::make_leaf_error(kraken::error_code::configuration_error, "asio receiver requires a non-null io_context");
        }
        return setup_receiver(receiver_cfg, *io_context);
      },
      [this, io_context](const ef_vi_udp_receiver_config& receiver_cfg) -> kraken::result<void> {
        if (io_context != nullptr) {
          return kraken::make_leaf_error(kraken::error_code::configuration_error, "ef_vi receiver does not take an io_context");
        }
        return setup_receiver(receiver_cfg);
      }));

  return {};
}

bool session::poll()
{
  if (asio_receiver_) {
    return io_context_->poll_one() > 0;
  }
  return ef_vi_receiver_->poll() > 0;
}

kraken::result<void> session::start()
{
  if (!session_.has_value()) {
    return kraken::make_leaf_error(kraken::error_code::configuration_error, "order_routing runtime session not set up");
  }

  if (asio_receiver_) {
    KRAKEN_LEAF_CHECK(asio_receiver_->start());
  } else if (ef_vi_receiver_) {
    KRAKEN_LEAF_CHECK(ef_vi_receiver_->open());
  }

  KRAKEN_LOG_INFO("order_routing runtime session started");
  return {};
}

void session::stop()
{
  if (asio_receiver_) {
    asio_receiver_->stop();
  }
  if (ef_vi_receiver_) {
    ef_vi_receiver_->close();
  }
}

kraken::result<void> session::setup_receiver(const asio_udp_receiver_config& config, boost::asio::io_context& io_context)
{
  io_context_ = &io_context;
  auto& receiver = asio_receiver_.emplace(io_context, config.endpoint);

  // The receiver hands us a view into its own buffer that is only valid
  // for the duration of this callback, so we forward synchronously.
  receiver.on_datagram = [this](kraken::network::types::datagram_view bytes) { session_->send(bytes); };

  return {};
}

kraken::result<void> session::setup_receiver(const ef_vi_udp_receiver_config& config)
{
  auto& receiver = ef_vi_receiver_.emplace(config.endpoint);

  receiver.on_datagram = [this](kraken::network::types::datagram_view bytes) { session_->send(bytes); };

  return {};
}

void session::setup_decoder(const csv_decoder_config&)
{
  decoder_ = std::make_unique<csv_decoder>();
}

} // namespace order_routing::runtime
