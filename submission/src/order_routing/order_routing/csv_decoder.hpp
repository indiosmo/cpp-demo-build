#ifndef ORDER_ROUTING_CSV_DECODER_HPP
#define ORDER_ROUTING_CSV_DECODER_HPP

#include "order_routing/decoder.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include "kraken/result.hpp"

#include <string_view>

/*
 * CSV decoder for the EXERCISE.md inbound wire protocol. Wire-grammar
 * violations (field count, marker validity, numeric parseability, side
 * tokens, symbol length) are treated as upstream-framer bugs and asserted
 * rather than returned as result-typed errors, since the session has no
 * meaningful runtime response to malformed input.
 */

namespace order_routing {

class csv_decoder final : public decoder
{
public:
  kraken::result<request> decode(std::string_view payload) const override;
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
