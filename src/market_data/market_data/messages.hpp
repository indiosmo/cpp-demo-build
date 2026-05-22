#ifndef MARKET_DATA_MESSAGES_HPP
#define MARKET_DATA_MESSAGES_HPP

#include "market_data/types.hpp"

#include <optional>
#include <variant>

/*
 * Market-data domain events model reference data, instrument state, trades,
 * and order-book updates. They do not carry order-entry lifecycle state.
 */

namespace market_data {

struct security_definition
{
  types::security_id security_id;
  types::symbol symbol;
  types::security_exchange security_exchange;
  types::security_group security_group;
  types::security_type security_type;
  std::optional<types::security_subtype> security_subtype;
  types::price min_price_increment;
  types::quantity round_lot;
  types::currency currency;
};

struct security_status
{
  types::security_id security_id;
  types::security_exchange security_exchange;
  types::trading_session_id trading_session_id;
  types::security_trading_status security_trading_status;
  types::security_trading_event security_trading_event;
  types::timestamp transact_time;
};

struct execution_summary
{
  types::security_id security_id;
  types::side aggressor_side;
  types::price last_px;
  types::quantity fill_qty;
  std::optional<types::quantity> traded_hidden_qty;
  std::optional<types::quantity> cancel_qty;
  types::timestamp aggressor_time;
  types::timestamp transact_time;
};

struct trade
{
  types::security_id security_id;
  types::trade_id trade_id;
  types::price price;
  types::quantity quantity;
  std::optional<types::order_id> buyer;
  std::optional<types::order_id> seller;
  types::trade_condition trade_condition;
  std::optional<types::trade_sub_type> trade_sub_type;
  types::trade_date trade_date;
  types::timestamp transact_time;
};

struct mbo_book_update
{
  types::security_id security_id;
  types::update_action update_action;
  types::side side;
  types::order_id resting_order_id;
  std::optional<types::price> price;
  std::optional<types::quantity> quantity;
  std::optional<types::quantity> previous_quantity;
  types::timestamp transact_time;
};

using message = std::variant<security_definition, security_status, execution_summary, trade, mbo_book_update>;

} // namespace market_data

#endif /* MARKET_DATA_MESSAGES_HPP */
