#ifndef MMD_JSON_JSON_ENCODER_HPP
#define MMD_JSON_JSON_ENCODER_HPP

#include "mmd/messages.hpp"

#include <string>

namespace mmd_json {

class json_encoder
{
public:
  [[nodiscard]] std::string encode(const mmd::message& message) const;
};

namespace detail {

const char* encode_side(mmd::types::side book_side);
const char* encode_update_action(mmd::types::update_action action);
const char* encode_security_trading_status(mmd::types::security_trading_status status);
const char* encode_security_trading_event(mmd::types::security_trading_event event);
const char* encode_trade_condition(mmd::types::trade_condition condition);
const char* encode_trade_sub_type(mmd::types::trade_sub_type trade_sub_type);

} // namespace detail

} // namespace mmd_json

#endif /* MMD_JSON_JSON_ENCODER_HPP */
