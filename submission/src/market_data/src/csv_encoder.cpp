#include "market_data/csv_encoder.hpp"

#include "kraken/fmt.hpp"
#include "kraken/variant.hpp"

#include <exception>
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

std::string encode_order_ack(const order_ack& msg)
{
  return fmt::format("A, {}, {}", msg.user, msg.order_id);
}

std::string encode_cancel_ack(const cancel_ack& msg)
{
  return fmt::format("C, {}, {}", msg.user, msg.order_id);
}

std::string encode_trade(const trade& msg)
{
  return fmt::format(
    "T, {}, {}, {}, {}, {}, {}", msg.buy_user, msg.buy_order, msg.sell_user, msg.sell_order, msg.trade_price, msg.trade_quantity);
}

namespace {

// Empty optional renders as "-" per the eliminated-side wire convention.
template <typename Optional>
std::string format_optional(const Optional& field)
{
  return field.has_value() ? std::to_string(field->get()) : "-";
}

} // namespace

std::string encode_top_of_book(const top_of_book& msg)
{
  return fmt::format(
    "B, {}, {}, {}", encode_side(msg.book_side), format_optional(msg.top_price), format_optional(msg.top_quantity));
}

} // namespace csv_encoder_detail

std::string csv_encoder::encode(const message& msg) const
{
  return kraken::match(
    msg,
    [](const order_ack& m) { return csv_encoder_detail::encode_order_ack(m); },
    [](const cancel_ack& m) { return csv_encoder_detail::encode_cancel_ack(m); },
    [](const trade& m) { return csv_encoder_detail::encode_trade(m); },
    [](const top_of_book& m) { return csv_encoder_detail::encode_top_of_book(m); });
}

} // namespace market_data
