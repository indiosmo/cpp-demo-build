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

char encode_update_action(types::update_action action);

std::string encode_security_definition(const security_definition& msg);

std::string encode_security_status(const security_status& msg);

std::string encode_execution_summary(const execution_summary& msg);

std::string encode_trade(const trade& msg);

std::string encode_mbo_book_update(const mbo_book_update& msg);

} // namespace csv_encoder_detail

} // namespace market_data

#endif /* MARKET_DATA_CSV_ENCODER_HPP */
