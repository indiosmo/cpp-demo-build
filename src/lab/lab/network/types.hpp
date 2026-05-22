#ifndef LAB_NETWORK_TYPES_HPP
#define LAB_NETWORK_TYPES_HPP

#include "lab/json.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace lab::network::types {

/* Bind target for a UDP receiver. "0.0.0.0" listens on every IPv4 interface. */
struct endpoint_config
{
  std::string address;
  std::uint16_t port = 0;
};

LAB_AUTO_JSON(endpoint_config, address, port)

/*
 * View over a received datagram's payload, valid only for the on_datagram
 * call that delivered it (the underlying buffer is the receiver's reusable
 * receive_buffer_). Typed as string_view because the JSON protocol is
 * text and downstream parses out of the view directly.
 */
using datagram_view = std::string_view;

} // namespace lab::network::types

#endif /* LAB_NETWORK_TYPES_HPP */
