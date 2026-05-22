#ifndef MARKET_DATA_CSV_ENCODER_HPP
#define MARKET_DATA_CSV_ENCODER_HPP

#include "market_data/encoder.hpp"
#include "market_data/messages.hpp"

#include <string>

/*
 * CSV encoder for the outbound CSV wire protocol. Per-record framing
 * (the trailing newline) is owned by the sink so a different wire format can
 * pick a different separator without touching the encoder.
 */

namespace market_data {

class csv_encoder final : public encoder
{
public:
  std::string encode(const message& msg) const override;
};

namespace csv_encoder_detail {

char encode_side(types::side book_side);

std::string encode_order_ack(const order_ack& msg);

std::string encode_cancel_ack(const cancel_ack& msg);

std::string encode_trade(const trade& msg);

std::string encode_top_of_book(const top_of_book& msg);

} // namespace csv_encoder_detail

} // namespace market_data

#endif /* MARKET_DATA_CSV_ENCODER_HPP */
