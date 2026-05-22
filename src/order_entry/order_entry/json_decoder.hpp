#ifndef ORDER_ENTRY_JSON_DECODER_HPP
#define ORDER_ENTRY_JSON_DECODER_HPP

#include "lab/json.hpp"
#include "lab/result.hpp"
#include "order_entry/decoder.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

/*
 * JSON decoder for the inbound order-entry wire protocol. Each datagram is
 * one object with a message_type discriminator and typed domain field names.
 */

namespace order_entry {

class json_decoder final : public decoder
{
public:
  lab::result<request> decode(std::string_view payload) const override;
};

namespace json_decoder_detail {

types::side decode_side(std::string_view field);

types::ord_type decode_ord_type(std::string_view field);

types::time_in_force decode_time_in_force(std::string_view field);

new_order_single decode_new_order(const lab::json::value& payload);

replace_order decode_replace_order(const lab::json::value& payload);

cancel_order decode_cancel_order(const lab::json::value& payload);

flush decode_flush(const lab::json::value& payload);

} // namespace json_decoder_detail

} // namespace order_entry

#endif /* ORDER_ENTRY_JSON_DECODER_HPP */
