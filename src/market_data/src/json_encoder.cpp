#include "market_data/json_encoder.hpp"

#include "lab/json.hpp"
#include "lab/variant.hpp"

#include <cstdlib>
#include <optional>
#include <string>

namespace market_data {

namespace json_encoder_detail {

const char* encode_side(types::side book_side)
{
  switch (book_side) {
    case types::side::buy:
      return "buy";
    case types::side::sell:
      return "sell";
  }

  std::terminate();
}

const char* encode_update_action(types::update_action action)
{
  switch (action) {
    case types::update_action::new_order:
      return "new_order";
    case types::update_action::change:
      return "change";
    case types::update_action::delete_order:
      return "delete_order";
  }

  std::terminate();
}

const char* encode_security_trading_status(types::security_trading_status status)
{
  switch (status) {
    case types::security_trading_status::open:
      return "open";
    case types::security_trading_status::closed:
      return "closed";
    case types::security_trading_status::halted:
      return "halted";
  }

  std::terminate();
}

const char* encode_security_trading_event(types::security_trading_event event)
{
  switch (event) {
    case types::security_trading_event::none:
      return "none";
    case types::security_trading_event::trading_resume:
      return "trading_resume";
    case types::security_trading_event::trading_halt:
      return "trading_halt";
  }

  std::terminate();
}

const char* encode_trade_condition(types::trade_condition condition)
{
  switch (condition) {
    case types::trade_condition::regular:
      return "regular";
  }

  std::terminate();
}

const char* encode_trade_sub_type(types::trade_sub_type trade_sub_type)
{
  switch (trade_sub_type) {
    case types::trade_sub_type::regular:
      return "regular";
  }

  std::terminate();
}

std::string encode_security_definition(const security_definition& msg)
{
  lab::json::value payload{
    {"message_type", "security_definition"},
    {"security_id", msg.security_id},
    {"symbol", msg.symbol},
    {"security_exchange", msg.security_exchange},
    {"security_group", msg.security_group},
    {"security_type", msg.security_type},
    {"security_subtype", msg.security_subtype},
    {"min_price_increment", msg.min_price_increment},
    {"round_lot", msg.round_lot},
    {"currency", msg.currency},
  };
  return lab::json::dump(payload);
}

std::string encode_security_status(const security_status& msg)
{
  lab::json::value payload{
    {"message_type", "security_status"},
    {"security_id", msg.security_id},
    {"security_exchange", msg.security_exchange},
    {"trading_session_id", msg.trading_session_id},
    {"security_trading_status", encode_security_trading_status(msg.security_trading_status)},
    {"security_trading_event", encode_security_trading_event(msg.security_trading_event)},
    {"transact_time", msg.transact_time},
  };
  return lab::json::dump(payload);
}

std::string encode_execution_summary(const execution_summary& msg)
{
  lab::json::value payload{
    {"message_type", "execution_summary"},
    {"security_id", msg.security_id},
    {"aggressor_side", encode_side(msg.aggressor_side)},
    {"last_px", msg.last_px},
    {"fill_qty", msg.fill_qty},
    {"traded_hidden_qty", msg.traded_hidden_qty},
    {"cancel_qty", msg.cancel_qty},
    {"aggressor_time", msg.aggressor_time},
    {"transact_time", msg.transact_time},
  };
  return lab::json::dump(payload);
}

std::string encode_trade(const trade& msg)
{
  lab::json::value payload{
    {"message_type", "trade"},
    {"security_id", msg.security_id},
    {"trade_id", msg.trade_id},
    {"price", msg.price},
    {"quantity", msg.quantity},
    {"buyer", msg.buyer},
    {"seller", msg.seller},
    {"trade_condition", encode_trade_condition(msg.trade_condition)},
    {"trade_sub_type",
     msg.trade_sub_type.has_value() ? std::optional<std::string>{encode_trade_sub_type(*msg.trade_sub_type)} : std::nullopt},
    {"trade_date", msg.trade_date},
    {"transact_time", msg.transact_time},
  };
  return lab::json::dump(payload);
}

std::string encode_mbo_book_update(const mbo_book_update& msg)
{
  lab::json::value payload{
    {"message_type", "mbo_book_update"},
    {"security_id", msg.security_id},
    {"update_action", encode_update_action(msg.update_action)},
    {"side", encode_side(msg.side)},
    {"resting_order_id", msg.resting_order_id},
    {"price", msg.price},
    {"quantity", msg.quantity},
    {"previous_quantity", msg.previous_quantity},
    {"transact_time", msg.transact_time},
  };
  return lab::json::dump(payload);
}

} // namespace json_encoder_detail

std::string json_encoder::encode(const message& msg) const
{
  return lab::match(
    msg,
    [](const security_definition& m) { return json_encoder_detail::encode_security_definition(m); },
    [](const security_status& m) { return json_encoder_detail::encode_security_status(m); },
    [](const execution_summary& m) { return json_encoder_detail::encode_execution_summary(m); },
    [](const trade& m) { return json_encoder_detail::encode_trade(m); },
    [](const mbo_book_update& m) { return json_encoder_detail::encode_mbo_book_update(m); });
}

} // namespace market_data
