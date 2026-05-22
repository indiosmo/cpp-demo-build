#include "mmd_json/json_encoder.hpp"

#include "lab/json.hpp"
#include "lab/variant.hpp"

#include <cstdlib>
#include <optional>
#include <string>

namespace mmd_json {

namespace detail {

const char* encode_side(mmd::types::side book_side)
{
  switch (book_side) {
    case mmd::types::side::buy:
      return "buy";
    case mmd::types::side::sell:
      return "sell";
  }

  std::terminate();
}

const char* encode_update_action(mmd::types::update_action action)
{
  switch (action) {
    case mmd::types::update_action::new_order:
      return "new_order";
    case mmd::types::update_action::change:
      return "change";
    case mmd::types::update_action::delete_order:
      return "delete_order";
  }

  std::terminate();
}

const char* encode_security_trading_status(mmd::types::security_trading_status status)
{
  switch (status) {
    case mmd::types::security_trading_status::open:
      return "open";
    case mmd::types::security_trading_status::closed:
      return "closed";
    case mmd::types::security_trading_status::halted:
      return "halted";
  }

  std::terminate();
}

const char* encode_security_trading_event(mmd::types::security_trading_event event)
{
  switch (event) {
    case mmd::types::security_trading_event::none:
      return "none";
    case mmd::types::security_trading_event::trading_resume:
      return "trading_resume";
    case mmd::types::security_trading_event::trading_halt:
      return "trading_halt";
  }

  std::terminate();
}

const char* encode_trade_condition(mmd::types::trade_condition condition)
{
  switch (condition) {
    case mmd::types::trade_condition::regular:
      return "regular";
  }

  std::terminate();
}

const char* encode_trade_sub_type(mmd::types::trade_sub_type trade_sub_type)
{
  switch (trade_sub_type) {
    case mmd::types::trade_sub_type::regular:
      return "regular";
  }

  std::terminate();
}

std::string encode_security_definition(const mmd::security_definition& msg)
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

std::string encode_security_status(const mmd::security_status& msg)
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

std::string encode_execution_summary(const mmd::execution_summary& msg)
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

std::string encode_trade(const mmd::trade& msg)
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

std::string encode_mbo_book_update(const mmd::mbo_book_update& msg)
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

} // namespace detail

std::string json_encoder::encode(const mmd::message& message) const
{
  return lab::match(
    message,
    [](const mmd::security_definition& msg) { return detail::encode_security_definition(msg); },
    [](const mmd::security_status& msg) { return detail::encode_security_status(msg); },
    [](const mmd::execution_summary& msg) { return detail::encode_execution_summary(msg); },
    [](const mmd::trade& msg) { return detail::encode_trade(msg); },
    [](const mmd::mbo_book_update& msg) { return detail::encode_mbo_book_update(msg); });
}

} // namespace mmd_json
