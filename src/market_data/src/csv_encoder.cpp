#include "market_data/csv_encoder.hpp"

#include "lab/fmt.hpp"
#include "lab/variant.hpp"

#include <exception>
#include <optional>
#include <string>

namespace market_data {

namespace csv_encoder_detail {

char encode_side(types::side book_side)
{
  switch (book_side) {
    case types::side::buy:
      return 'B';
    case types::side::sell:
      return 'S';
  }

  std::terminate(); // unreachable: enum is exhaustive.
}

char encode_update_action(types::update_action action)
{
  switch (action) {
    case types::update_action::new_order:
      return 'N';
    case types::update_action::change:
      return 'C';
    case types::update_action::delete_order:
      return 'D';
  }

  std::terminate();
}

std::string encode_security_definition(const security_definition& msg)
{
  return fmt::format(
    "D, {}, {}, {}, {}, {}, {}, {}, {}",
    msg.security_id,
    msg.symbol,
    msg.security_exchange,
    msg.security_group,
    msg.security_type,
    msg.min_price_increment,
    msg.round_lot,
    msg.currency);
}

std::string encode_security_status(const security_status& msg)
{
  return fmt::format(
    "S, {}, {}, {}, {}, {}, {}",
    msg.security_id,
    msg.security_exchange,
    msg.trading_session_id,
    static_cast<unsigned>(msg.security_trading_status),
    static_cast<unsigned>(msg.security_trading_event),
    msg.transact_time);
}

std::string encode_execution_summary(const execution_summary& msg)
{
  return fmt::format(
    "E, {}, {}, {}, {}, {}, {}",
    msg.security_id,
    encode_side(msg.aggressor_side),
    msg.last_px,
    msg.fill_qty,
    msg.aggressor_time,
    msg.transact_time);
}

std::string encode_trade(const trade& msg)
{
  return fmt::format(
    "T, {}, {}, {}, {}, {}, {}, {}, {}",
    msg.security_id,
    msg.trade_id,
    msg.price,
    msg.quantity,
    msg.buyer.has_value() ? std::to_string(msg.buyer->get()) : "-",
    msg.seller.has_value() ? std::to_string(msg.seller->get()) : "-",
    static_cast<unsigned>(msg.trade_condition),
    msg.transact_time);
}

namespace {

// Empty optional renders as "-" per the eliminated-side wire convention.
template <typename Optional>
std::string format_optional(const Optional& field)
{
  return field.has_value() ? std::to_string(field->get()) : "-";
}

} // namespace

std::string encode_mbo_book_update(const mbo_book_update& msg)
{
  return fmt::format(
    "M, {}, {}, {}, {}, {}, {}, {}, {}",
    msg.security_id,
    encode_update_action(msg.update_action),
    encode_side(msg.side),
    msg.resting_order_id,
    format_optional(msg.price),
    format_optional(msg.quantity),
    format_optional(msg.previous_quantity),
    msg.transact_time);
}

} // namespace csv_encoder_detail

std::string csv_encoder::encode(const message& msg) const
{
  return lab::match(
    msg,
    [](const security_definition& m) { return csv_encoder_detail::encode_security_definition(m); },
    [](const security_status& m) { return csv_encoder_detail::encode_security_status(m); },
    [](const execution_summary& m) { return csv_encoder_detail::encode_execution_summary(m); },
    [](const trade& m) { return csv_encoder_detail::encode_trade(m); },
    [](const mbo_book_update& m) { return csv_encoder_detail::encode_mbo_book_update(m); });
}

} // namespace market_data
