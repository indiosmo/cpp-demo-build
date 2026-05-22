#ifndef ORDER_ROUTING_CSV_DECODER_HPP
#define ORDER_ROUTING_CSV_DECODER_HPP

#include "order_routing/decoder.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include "lab/result.hpp"

#include <string_view>

/*
 * CSV decoder for the inbound wire protocol. The decoder relies on fixed-shape
 * records and asserts local grammar preconditions: field count, marker
 * validity, numeric parseability, side tokens, and symbol length.
 */

namespace order_routing {

class csv_decoder final : public decoder
{
public:
  lab::result<request> decode(std::string_view payload) const override;
};

namespace csv_decoder_detail {

// precondition: field is "B" or "S"
types::side parse_side(std::string_view field);

types::symbol parse_symbol(std::string_view field);

new_order decode_new_order(std::string_view payload);

cancel_order decode_cancel_order(std::string_view payload);

// precondition: payload is empty
flush decode_flush(std::string_view payload);

} // namespace csv_decoder_detail

} // namespace order_routing

#endif /* ORDER_ROUTING_CSV_DECODER_HPP */
