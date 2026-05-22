#ifndef LAB_NETWORK_EF_VI_UDP_RECEIVER_HPP
#define LAB_NETWORK_EF_VI_UDP_RECEIVER_HPP

#include "lab/inplace_function.hpp"
#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include <cstddef>

namespace lab::network {

/*
 * Stub Solarflare ef_vi kernel-bypass UDP receiver. Kept separate from
 * asio_udp_receiver because the scheduling models diverge: ef_vi is
 * poll-driven (caller owns the busy loop), asio is callback-driven on
 * io_context::run(). A shared abstract base would have to fake one model on
 * top of the other, so the wiring shell picks a concrete backend at
 * composition time.
 *
 * IMPROVEMENT: real integration -- allocate a virtual interface,
 * register an RX filter, post receive descriptors, parse
 * Ethernet/IP/UDP off raw frames per poll().
 */
class ef_vi_udp_receiver
{
public:
  explicit ef_vi_udp_receiver(types::endpoint_config config);
  ef_vi_udp_receiver(const ef_vi_udp_receiver&) = delete;
  ef_vi_udp_receiver(ef_vi_udp_receiver&&) = delete;
  ef_vi_udp_receiver& operator=(const ef_vi_udp_receiver&) = delete;
  ef_vi_udp_receiver& operator=(ef_vi_udp_receiver&&) = delete;
  ~ef_vi_udp_receiver() = default;

  /* Stub: returns lab::error_code::not_implemented. */
  lab::result<void> open();

  /* Stub: returns 0. */
  std::size_t poll();

  /* Stub: no-op. */
  void close();

  /*
   * Invoked for each datagram drained by poll(); view is valid only for the
   * duration of the call.
   */
  lab::inplace_function<void(types::datagram_view)> on_datagram;

private:
  types::endpoint_config config_;
};

} // namespace lab::network

#endif /* LAB_NETWORK_EF_VI_UDP_RECEIVER_HPP */
