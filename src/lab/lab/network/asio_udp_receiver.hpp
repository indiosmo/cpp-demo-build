#ifndef LAB_NETWORK_ASIO_UDP_RECEIVER_HPP
#define LAB_NETWORK_ASIO_UDP_RECEIVER_HPP

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/udp.hpp"

#include "lab/inplace_function.hpp"
#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include <array>
#include <cstddef>

namespace lab::network {

/*
 * Boost.Asio-backed UDP receiver. The io_context is injected so the wiring
 * shell can mux this receiver onto an event-loop thread shared with other
 * asio work. The receive buffer is sized to the largest IPv4 UDP payload and
 * reused across receives. See ef_vi_udp_receiver.hpp for why the two backends
 * stay independent concrete types rather than sharing an abstract base.
 */
class asio_udp_receiver
{
public:
  asio_udp_receiver(boost::asio::io_context& io_context, types::endpoint_config config);
  asio_udp_receiver(const asio_udp_receiver&) = delete;
  asio_udp_receiver(asio_udp_receiver&&) = delete;
  asio_udp_receiver& operator=(const asio_udp_receiver&) = delete;
  asio_udp_receiver& operator=(asio_udp_receiver&&) = delete;
  ~asio_udp_receiver() = default;

  /*
   * Binds the socket and arms the receive loop. The handler fires when the
   * injected io_context is polled.
   */
  lab::result<void> start();

  /*
   * Cancels outstanding receives; safe to call from a thread other than the
   * one driving the io_context.
   */
  void stop();

  /*
   * Invoked for every successfully received datagram; the view is valid only
   * for the duration of the call.
   */
  lab::inplace_function<void(types::datagram_view)> on_datagram;

private:
  void post_receive();

  // Maximum IPv4 UDP payload.
  static constexpr std::size_t receive_buffer_size = 65535;

  types::endpoint_config config_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  std::array<char, receive_buffer_size> receive_buffer_{};
};

} // namespace lab::network

#endif /* LAB_NETWORK_ASIO_UDP_RECEIVER_HPP */
