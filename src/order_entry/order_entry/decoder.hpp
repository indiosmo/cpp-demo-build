#ifndef ORDER_ENTRY_DECODER_HPP
#define ORDER_ENTRY_DECODER_HPP

#include "order_entry/messages.hpp"

#include "lab/result.hpp"

#include <string_view>

/*
 * Abstract decoder boundary: one wire packet to one typed request. Keeps
 * the session stage agnostic of the wire format and lets tests substitute
 * a scripted decoder.
 */

namespace order_entry {

class decoder
{
public:
  virtual ~decoder() = default;

  virtual lab::result<request> decode(std::string_view payload) const = 0;
};

} // namespace order_entry

#endif /* ORDER_ENTRY_DECODER_HPP */
