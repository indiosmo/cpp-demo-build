#ifndef ORDER_ROUTING_SESSION_HPP
#define ORDER_ROUTING_SESSION_HPP

#include "order_routing/decoder.hpp"
#include "order_routing/messages.hpp"

#include "lab/inplace_function.hpp"

#include <string_view>

/*
 * Pipeline stage that turns raw packets into typed requests or
 * rejections. Pure synchronous code over value types; the decoder is
 * injected by reference, and on_request / on_rejected must be wired
 * before the first send().
 */

namespace order_routing {

class session
{
public:
  explicit session(const decoder& packet_decoder);

  void send(std::string_view packet);

  lab::inplace_function<void(const request&)> on_request;
  lab::inplace_function<void(const rejection&)> on_rejected;

private:
  const decoder* decoder_;
};

} // namespace order_routing

#endif /* ORDER_ROUTING_SESSION_HPP */
