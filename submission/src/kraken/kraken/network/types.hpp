#ifndef KRAKEN_NETWORK_TYPES_HPP
#define KRAKEN_NETWORK_TYPES_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace kraken::network::types {

/* Bind target for a UDP receiver. "0.0.0.0" listens on every IPv4 interface. */
struct endpoint_config
{
  std::string address;
  std::uint16_t port = 0;
};

/*
 * View over a received datagram's payload, valid only for the on_datagram
 * call that delivered it (the underlying buffer is the receiver's reusable
 * receive_buffer_). Typed as string_view because the submission protocol is
 * CSV text and downstream parses out of the view directly.
 */
using datagram_view = std::string_view;

} // namespace kraken::network::types

#endif /* KRAKEN_NETWORK_TYPES_HPP */
