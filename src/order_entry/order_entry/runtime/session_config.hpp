#ifndef ORDER_ENTRY_RUNTIME_SESSION_CONFIG_HPP
#define ORDER_ENTRY_RUNTIME_SESSION_CONFIG_HPP

#include "lab/network/types.hpp"

#include <variant>

/*
 * Configuration surface for order_entry::runtime::session. Backend
 * choice is a variant: a new UDP transport or decoder slots in as a new
 * alternative plus a matching setup overload in the runtime.
 */

namespace order_entry::runtime {

// Boost.Asio kernel-socket receiver. The wiring shell owns the io_context
// and drives its poll tick from the receive event loop, so the receiver
// does not need its own thread.
struct asio_udp_receiver_config
{
  lab::network::types::endpoint_config endpoint;
};

// Solarflare ef_vi kernel-bypass receiver. The implementation is a stub;
// the seat exercises the poll-driven branch of the composer.
struct ef_vi_udp_receiver_config
{
  lab::network::types::endpoint_config endpoint;
};

using receiver_config = std::variant<asio_udp_receiver_config, ef_vi_udp_receiver_config>;

struct json_decoder_config
{
};

using decoder_config = std::variant<json_decoder_config>;

struct session_config
{
  receiver_config receiver;
  decoder_config decoder{json_decoder_config{}};
};

} // namespace order_entry::runtime

#endif /* ORDER_ENTRY_RUNTIME_SESSION_CONFIG_HPP */
