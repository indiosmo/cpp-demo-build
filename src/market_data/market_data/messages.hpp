#ifndef MARKET_DATA_MESSAGES_HPP
#define MARKET_DATA_MESSAGES_HPP

#include "market_data/types.hpp"

#include <optional>
#include <variant>

/*
 * Outbound market data messages produced by the matching engine, mirroring
 * the outbound CSV protocol:
 *   A, userId, userOrderId                                                -> order_ack
 *   C, userId, userOrderId                                                -> cancel_ack
 *   T, userIdBuy, userOrderIdBuy, userIdSell, userOrderIdSell, price, qty -> trade
 *   B, side(B|S), price, totalQuantity                                    -> top_of_book
 *
 * top_of_book uses std::optional rather than a sentinel so the encoder
 * branches on present/absent; nullopt renders as '-'.
 */

namespace market_data {

struct order_ack
{
  types::user_id user;
  types::user_order_id order_id;
};

struct cancel_ack
{
  types::user_id user;
  types::user_order_id order_id;
};

struct trade
{
  types::user_id buy_user;
  types::user_order_id buy_order;
  types::user_id sell_user;
  types::user_order_id sell_order;
  types::price trade_price;
  types::quantity trade_quantity;
};

struct top_of_book
{
  types::side book_side;
  std::optional<types::price> top_price;
  std::optional<types::total_quantity> top_quantity;
};

using message = std::variant<order_ack, cancel_ack, trade, top_of_book>;

} // namespace market_data

#endif /* MARKET_DATA_MESSAGES_HPP */
