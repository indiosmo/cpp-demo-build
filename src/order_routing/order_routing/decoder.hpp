#ifndef ORDER_ROUTING_DECODER_HPP
#define ORDER_ROUTING_DECODER_HPP

#include "order_routing/messages.hpp"

#include "lab/result.hpp"

#include <string_view>

/*
 * Abstract decoder boundary: one wire packet to one typed request. Keeps
 * the session stage agnostic of the wire format and lets tests substitute
 * a scripted decoder.
 */

namespace order_routing {

class decoder
{
public:
  virtual ~decoder() = default;

  virtual lab::result<request> decode(std::string_view payload) const = 0;
};

} // namespace order_routing

#endif /* ORDER_ROUTING_DECODER_HPP */
