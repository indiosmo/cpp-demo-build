#ifndef ORDER_ENTRY_SESSION_HPP
#define ORDER_ENTRY_SESSION_HPP

#include "order_entry/decoder.hpp"
#include "order_entry/messages.hpp"

#include "lab/inplace_function.hpp"

#include <string_view>

/*
 * Pipeline stage that turns raw packets into typed requests or
 * rejections. Pure synchronous code over value types; the decoder is
 * injected by reference, and on_request / on_rejected must be wired
 * before the first send().
 */

namespace order_entry {

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

} // namespace order_entry

#endif /* ORDER_ENTRY_SESSION_HPP */
