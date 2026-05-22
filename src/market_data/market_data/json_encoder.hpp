#ifndef MARKET_DATA_JSON_ENCODER_HPP
#define MARKET_DATA_JSON_ENCODER_HPP

#include "market_data/encoder.hpp"
#include "market_data/messages.hpp"

#include <string>

namespace market_data {

class json_encoder final : public encoder
{
public:
  std::string encode(const message& msg) const override;
};

namespace json_encoder_detail {

const char* encode_side(types::side book_side);

const char* encode_update_action(types::update_action action);

const char* encode_security_trading_status(types::security_trading_status status);

const char* encode_security_trading_event(types::security_trading_event event);

const char* encode_trade_condition(types::trade_condition condition);

const char* encode_trade_sub_type(types::trade_sub_type trade_sub_type);

std::string encode_security_definition(const security_definition& msg);

std::string encode_security_status(const security_status& msg);

std::string encode_execution_summary(const execution_summary& msg);

std::string encode_trade(const trade& msg);

std::string encode_mbo_book_update(const mbo_book_update& msg);

} // namespace json_encoder_detail

} // namespace market_data

#endif /* MARKET_DATA_JSON_ENCODER_HPP */
