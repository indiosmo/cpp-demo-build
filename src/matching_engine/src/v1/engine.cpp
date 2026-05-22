#include "matching_engine/v1/engine.hpp"

#include "matching_engine/conversions.hpp"
#include "matching_engine/factories.hpp"

#include "lab/log.hpp"
#include "lab/variant.hpp"

#include <algorithm>
#include <optional>

namespace matching_engine::v1 {

namespace md = market_data;
namespace rt = order_entry;

namespace {

constexpr bool is_market_order(const order_state& taker)
{
  return taker.price == 0;
}

constexpr bool taker_crosses_bid(const order_state& taker, rt::types::price resting_bid)
{
  return is_market_order(taker) || taker.price <= resting_bid;
}

constexpr bool taker_crosses_ask(const order_state& taker, rt::types::price resting_ask)
{
  return is_market_order(taker) || taker.price >= resting_ask;
}

} // namespace

engine::engine(engine_config configuration)
{
  books_.reserve(configuration.valid_symbols.size());
  for (const auto& symbol : configuration.valid_symbols) {
    books_.try_emplace(symbol);
  }
  resting_index_.reserve(configuration.expected_resting_orders);
}

void engine::send(const rt::request& cmd)
{
  lab::match(cmd, [this](const auto& specific) { handle(specific); });
}

void engine::handle(const rt::new_order_single& incoming)
{
  const types::order_key incoming_key{.client_id = incoming.client_id, .cl_ord_id = incoming.cl_ord_id};
  if (resting_index_.contains(incoming_key)) {
    LAB_LOG_WARN(
      "rejecting new_order_single: duplicate (client_id={}, cl_ord_id={}) already resting", incoming.client_id, incoming.cl_ord_id);
    return;
  }

  const auto book_it = books_.find(incoming.symbol);
  if (book_it == books_.end()) {
    LAB_LOG_WARN("rejecting new_order_single: unknown symbol {}", incoming.symbol);
    return;
  }
  auto& book = book_it->second;

  on_event(
    md::order_ack{
      .client_id = md::types::client_id{incoming.client_id},
      .cl_ord_id = md::types::cl_ord_id{incoming.cl_ord_id},
    });

  order_state taker = make_order_state(incoming);

  switch (incoming.side) {
    case rt::types::side::buy:
      match_buy(book, taker);
      break;

    case rt::types::side::sell:
      match_sell(book, taker);
      break;
  }

  const bool had_trades = taker.leaves_qty < incoming.order_qty;
  const bool placed_residual = taker.leaves_qty > 0 && !is_market_order(taker);

  if (placed_residual) {
    book.place(taker);
    resting_index_.emplace(incoming_key, incoming.symbol);
  }

  const rt::types::side opposite_side =
    (incoming.side == rt::types::side::buy) ? rt::types::side::sell : rt::types::side::buy;

  if (had_trades) {
    emit_top_of_book(book, opposite_side);
  }

  if (placed_residual) {
    const auto own_best = (incoming.side == rt::types::side::buy) ? book.best_bid() : book.best_ask();
    if (own_best == incoming.price) {
      emit_top_of_book(book, incoming.side);
    }
  }
}

void engine::handle(const rt::cancel_order& incoming)
{
  const types::order_key key{.client_id = incoming.client_id, .cl_ord_id = incoming.cl_ord_id};
  const auto index_it = resting_index_.find(key);
  if (index_it == resting_index_.end()) {
    return;
  }

  const auto book_it = books_.find(index_it->second);
  if (book_it == books_.end()) {
    resting_index_.erase(index_it);
    return;
  }
  auto& book = book_it->second;

  const std::optional<rt::types::price> bid_top_before = book.best_bid();
  const std::optional<rt::types::price> ask_top_before = book.best_ask();

  auto removed = book.cancel(incoming.client_id, incoming.cl_ord_id);
  if (!removed) {
    resting_index_.erase(index_it);
    return;
  }

  resting_index_.erase(index_it);

  on_event(
    md::cancel_ack{
      .client_id = md::types::client_id{incoming.client_id},
      .cl_ord_id = md::types::cl_ord_id{incoming.cl_ord_id},
    });

  const auto& cancelled = removed.value();
  const std::optional<rt::types::price> top_before =
    (cancelled.side == rt::types::side::buy) ? bid_top_before : ask_top_before;

  if (top_before == cancelled.price) {
    emit_top_of_book(book, cancelled.side);
  }
}

void engine::handle(const rt::flush&)
{
  for (auto& book_entry : books_) {
    book_entry.second = order_book{};
  }
  resting_index_.clear();
}

void engine::match_buy(order_book& book, order_state& taker)
{
  auto& asks = book.asks();

  while (taker.leaves_qty > 0 && !asks.empty()) {
    auto level_it = asks.begin();
    const rt::types::price resting_ask = level_it->first;
    if (!taker_crosses_ask(taker, resting_ask)) {
      break;
    }

    auto& level = level_it->second;
    while (taker.leaves_qty > 0 && !level.empty()) {
      order_state& maker = level.front();
      const rt::types::quantity fill = std::min(taker.leaves_qty, maker.leaves_qty);

      on_event(
        md::trade{
          .buy_user = md::types::client_id{taker.client_id},
          .buy_order = md::types::cl_ord_id{taker.cl_ord_id},
          .sell_user = md::types::client_id{maker.client_id},
          .sell_order = md::types::cl_ord_id{maker.cl_ord_id},
          .trade_price = md::types::price{resting_ask},
          .trade_quantity = md::types::quantity{fill},
        });

      taker.leaves_qty -= fill;
      maker.leaves_qty -= fill;

      if (maker.leaves_qty == 0) {
        const types::order_key maker_key{.client_id = maker.client_id, .cl_ord_id = maker.cl_ord_id};
        level.erase(level.begin());
        resting_index_.erase(maker_key);
      }
    }

    if (level.empty()) {
      asks.erase(level_it);
    }
  }
}

void engine::match_sell(order_book& book, order_state& taker)
{
  auto& bids = book.bids();

  while (taker.leaves_qty > 0 && !bids.empty()) {
    auto level_it = bids.begin();
    const rt::types::price resting_bid = level_it->first;
    if (!taker_crosses_bid(taker, resting_bid)) {
      break;
    }

    auto& level = level_it->second;
    while (taker.leaves_qty > 0 && !level.empty()) {
      order_state& maker = level.front();
      const rt::types::quantity fill = std::min(taker.leaves_qty, maker.leaves_qty);

      on_event(
        md::trade{
          .buy_user = md::types::client_id{maker.client_id},
          .buy_order = md::types::cl_ord_id{maker.cl_ord_id},
          .sell_user = md::types::client_id{taker.client_id},
          .sell_order = md::types::cl_ord_id{taker.cl_ord_id},
          .trade_price = md::types::price{resting_bid},
          .trade_quantity = md::types::quantity{fill},
        });

      taker.leaves_qty -= fill;
      maker.leaves_qty -= fill;

      if (maker.leaves_qty == 0) {
        const types::order_key maker_key{.client_id = maker.client_id, .cl_ord_id = maker.cl_ord_id};
        level.erase(level.begin());
        resting_index_.erase(maker_key);
      }
    }

    if (level.empty()) {
      bids.erase(level_it);
    }
  }
}

void engine::emit_top_of_book(const order_book& book, rt::types::side affected_side)
{
  std::optional<rt::types::price> top_price;
  std::optional<rt::types::quantity> total_quantity;

  switch (affected_side) {
    case rt::types::side::buy:
      top_price = book.best_bid();
      total_quantity = book.total_at_best_bid();
      break;

    case rt::types::side::sell:
      top_price = book.best_ask();
      total_quantity = book.total_at_best_ask();
      break;
  }

  std::optional<md::types::price> outbound_top_price;
  if (top_price.has_value()) {
    outbound_top_price = md::types::price{*top_price};
  }

  std::optional<md::types::total_quantity> outbound_top_quantity;
  if (total_quantity.has_value()) {
    outbound_top_quantity = md::types::total_quantity{*total_quantity};
  }

  on_event(
    md::top_of_book{
      .book_side = to_market_side(affected_side),
      .top_price = outbound_top_price,
      .top_quantity = outbound_top_quantity,
    });
}

} // namespace matching_engine::v1
