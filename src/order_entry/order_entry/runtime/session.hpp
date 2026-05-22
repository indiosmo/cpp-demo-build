#ifndef ORDER_ENTRY_RUNTIME_SESSION_HPP
#define ORDER_ENTRY_RUNTIME_SESSION_HPP

#include "boost/asio/io_context.hpp"
#include "order_entry/decoder.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/runtime/session_config.hpp"
#include "order_entry/session.hpp"

#include "lab/inplace_function.hpp"
#include "lab/network/asio_udp_receiver.hpp"
#include "lab/network/ef_vi_udp_receiver.hpp"
#include "lab/result.hpp"

#include <memory>
#include <optional>

/*
 * Runtime composer for the order_entry pipeline stage: owns the UDP
 * receiver, decoder, and synchronous session, and re-exports the inner
 * session's callbacks. Keeps the inner order_entry::session thread-free
 * and unit-testable; the wiring shell sees only start/stop/poll.
 */

namespace order_entry::runtime {

class session
{
public:
  // Invoked on the receive-loop thread.
  lab::inplace_function<void(const order_entry::request&)> on_request;
  lab::inplace_function<void(const order_entry::rejection&)> on_rejected;

  session() = default;
  session(const session&) = delete;
  session(session&&) = delete;
  session& operator=(const session&) = delete;
  session& operator=(session&&) = delete;
  ~session();

  // Precondition: io_context is non-null iff config.receiver holds
  // asio_udp_receiver_config, and outlives this session.
  lab::result<void> setup(const session_config& config, boost::asio::io_context* io_context);

  // Returns true when work was performed. Precondition: setup() succeeded.
  bool poll();

  lab::result<void> start();

  // Safe to call before start() and from any thread.
  void stop();

private:
  lab::result<void> setup_receiver(const asio_udp_receiver_config& config, boost::asio::io_context& io_context);
  lab::result<void> setup_receiver(const ef_vi_udp_receiver_config& config);

  void setup_decoder(const csv_decoder_config& config);

  // Non-owning; set in the asio setup_receiver overload, null on the ef_vi
  // branch.
  boost::asio::io_context* io_context_ = nullptr;

  // Exactly one is engaged after setup().
  std::optional<lab::network::asio_udp_receiver> asio_receiver_;
  std::optional<lab::network::ef_vi_udp_receiver> ef_vi_receiver_;

  std::unique_ptr<decoder> decoder_;

  // Declared after decoder_ so it tears down first and *decoder_ stays
  // alive while it does.
  std::optional<order_entry::session> session_;
};

} // namespace order_entry::runtime

#endif /* ORDER_ENTRY_RUNTIME_SESSION_HPP */
