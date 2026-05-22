#ifndef ORDER_ENTRY_CSV_DECODER_HPP
#define ORDER_ENTRY_CSV_DECODER_HPP

#include "order_entry/decoder.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

#include "lab/result.hpp"

#include <string_view>

/*
 * CSV decoder for the inbound wire protocol. The decoder relies on fixed-shape
 * records and asserts local grammar preconditions: field count, marker
 * validity, numeric parseability, side tokens, and symbol length.
 */

namespace order_entry {

class csv_decoder final : public decoder
{
public:
  lab::result<request> decode(std::string_view payload) const override;
};

namespace csv_decoder_detail {

// precondition: field is "B" or "S"
types::side parse_side(std::string_view field);

types::symbol parse_symbol(std::string_view field);

new_order_single decode_new_order(std::string_view payload);

replace_order decode_replace_order(std::string_view payload);

cancel_order decode_cancel_order(std::string_view payload);

// precondition: payload is empty
flush decode_flush(std::string_view payload);

} // namespace csv_decoder_detail

} // namespace order_entry

#endif /* ORDER_ENTRY_CSV_DECODER_HPP */
