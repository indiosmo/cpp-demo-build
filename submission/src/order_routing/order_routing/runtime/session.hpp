#ifndef ORDER_ROUTING_RUNTIME_SESSION_HPP
#define ORDER_ROUTING_RUNTIME_SESSION_HPP

#include "boost/asio/io_context.hpp"
#include "order_routing/decoder.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/runtime/session_config.hpp"
#include "order_routing/session.hpp"

#include "kraken/inplace_function.hpp"
#include "kraken/network/asio_udp_receiver.hpp"
#include "kraken/network/ef_vi_udp_receiver.hpp"
#include "kraken/result.hpp"

#include <memory>
#include <optional>

/*
 * Runtime composer for the order_routing pipeline stage: owns the UDP
 * receiver, decoder, and synchronous session, and re-exports the inner
 * session's callbacks. Keeps the inner order_routing::session thread-free
 * and unit-testable; the wiring shell sees only start/stop/poll.
 */

namespace order_routing::runtime {

class session
{
public:
  // Invoked on the receive-loop thread.
  kraken::inplace_function<void(const order_routing::request&)> on_request;
  kraken::inplace_function<void(const order_routing::rejection&)> on_rejected;

  session() = default;
  session(const session&) = delete;
  session(session&&) = delete;
  session& operator=(const session&) = delete;
  session& operator=(session&&) = delete;
  ~session();

  // Precondition: io_context is non-null iff config.receiver holds
  // asio_udp_receiver_config, and outlives this session.
  kraken::result<void> setup(const session_config& config, boost::asio::io_context* io_context);

  // Returns true when work was performed. Precondition: setup() succeeded.
  bool poll();

  kraken::result<void> start();

  // Safe to call before start() and from any thread.
  void stop();

private:
  kraken::result<void> setup_receiver(const asio_udp_receiver_config& config, boost::asio::io_context& io_context);
  kraken::result<void> setup_receiver(const ef_vi_udp_receiver_config& config);

  void setup_decoder(const csv_decoder_config& config);

  // Non-owning; set in the asio setup_receiver overload, null on the ef_vi
  // branch.
  boost::asio::io_context* io_context_ = nullptr;

  // Exactly one is engaged after setup().
  std::optional<kraken::network::asio_udp_receiver> asio_receiver_;
  std::optional<kraken::network::ef_vi_udp_receiver> ef_vi_receiver_;

  std::unique_ptr<decoder> decoder_;

  // Declared after decoder_ so it tears down first and *decoder_ stays
  // alive while it does.
  std::optional<order_routing::session> session_;
};

} // namespace order_routing::runtime

#endif /* ORDER_ROUTING_RUNTIME_SESSION_HPP */
