#include "matching_engine/v2/engine.hpp"

#include "matching_engine/conversions.hpp"
#include "matching_engine/factories.hpp"

#include "lab/assert.hpp"
#include "lab/log.hpp"
#include "lab/variant.hpp"

#include <algorithm>
#include <optional>

namespace matching_engine::v2 {

namespace md = market_data;
namespace rt = order_routing;

namespace {

constexpr bool is_market_order(const rt::new_order& taker)
{
  return taker.limit_price == 0;
}

constexpr bool taker_crosses_bid(const rt::new_order& taker, rt::types::price resting_bid)
{
  return is_market_order(taker) || taker.limit_price <= resting_bid;
}

constexpr bool taker_crosses_ask(const rt::new_order& taker, rt::types::price resting_ask)
{
  return is_market_order(taker) || taker.limit_price >= resting_ask;
}

} // namespace

engine::engine(engine_config configuration)
  : node_pool_{sizeof(order_node), configuration.node_pool_chunk_size}
{
  books_.reserve(configuration.valid_symbols.size());
  for (const auto& instrument : configuration.valid_symbols) {
    books_.try_emplace(instrument, node_pool_);
  }
  resting_index_.reserve(configuration.expected_resting_orders);
}

void engine::send(const rt::request& cmd)
{
  lab::match(cmd, [this](const auto& specific) { handle(specific); });
}

void engine::handle(const rt::new_order& incoming)
{
  const types::order_key incoming_key{.user = incoming.user, .order_id = incoming.order_id};
  if (resting_index_.contains(incoming_key)) {
    LAB_LOG_WARN(
      "rejecting new_order: duplicate (user={}, user_order_id={}) already resting", incoming.user, incoming.order_id);
    return;
  }

  const auto book_it = books_.find(incoming.instrument);
  if (book_it == books_.end()) {
    LAB_LOG_WARN("rejecting new_order: unknown symbol {}", incoming.instrument);
    return;
  }
  auto& book = book_it->second;

  on_event(
    md::order_ack{
      .user = md::types::user_id{incoming.user},
      .order_id = md::types::user_order_id{incoming.order_id},
    });

  rt::types::quantity remaining = incoming.order_quantity;

  switch (incoming.order_side) {
    case rt::types::side::buy:
      match_buy(book, incoming, remaining);
      break;

    case rt::types::side::sell:
      match_sell(book, incoming, remaining);
      break;
  }

  const bool had_trades = remaining < incoming.order_quantity;
  const bool placed_residual = remaining > 0 && !is_market_order(incoming);

  if (placed_residual) {
    order_node* node = book.place(
      order_state{
        .user = incoming.user,
        .order_id = incoming.order_id,
        .instrument = incoming.instrument,
        .order_side = incoming.order_side,
        .limit_price = incoming.limit_price,
        .remaining_quantity = remaining,
      });
    resting_index_.emplace(incoming_key, node);
  }

  const rt::types::side opposite_side =
    (incoming.order_side == rt::types::side::buy) ? rt::types::side::sell : rt::types::side::buy;

  if (had_trades) {
    emit_top_of_book(book, opposite_side);
  }

  if (placed_residual) {
    const auto own_best = (incoming.order_side == rt::types::side::buy) ? book.best_bid() : book.best_ask();
    if (own_best == incoming.limit_price) {
      emit_top_of_book(book, incoming.order_side);
    }
  }
}

void engine::handle(const rt::cancel_order& incoming)
{
  const types::order_key key{.user = incoming.user, .order_id = incoming.order_id};
  const auto index_it = resting_index_.find(key);
  if (index_it == resting_index_.end()) {
    return;
  }

  order_node* const node = index_it->second;
  const order_state resting = node->data;

  const auto book_it = books_.find(resting.instrument);
  LAB_ASSERT(book_it != books_.end());
  auto& book = book_it->second;

  const std::optional<rt::types::price> top_before =
    (resting.order_side == rt::types::side::buy) ? book.best_bid() : book.best_ask();

  book.cancel(node);
  resting_index_.erase(index_it);

  on_event(
    md::cancel_ack{
      .user = md::types::user_id{incoming.user},
      .order_id = md::types::user_order_id{incoming.order_id},
    });

  if (top_before == resting.limit_price) {
    emit_top_of_book(book, resting.order_side);
  }
}

void engine::handle(const rt::flush&)
{
  for (auto& book_entry : books_) {
    auto& book = book_entry.second;
    while (book.best_bid().has_value()) {
      const order_state& front = book.top_bid_front();
      resting_index_.erase(types::order_key{.user = front.user, .order_id = front.order_id});
      const auto consumed = book.fill_top_bid_front(front.remaining_quantity);
      LAB_ASSERT(consumed.has_value());
    }
    while (book.best_ask().has_value()) {
      const order_state& front = book.top_ask_front();
      resting_index_.erase(types::order_key{.user = front.user, .order_id = front.order_id});
      const auto consumed = book.fill_top_ask_front(front.remaining_quantity);
      LAB_ASSERT(consumed.has_value());
    }
  }
}

void engine::match_buy(order_book& book, const rt::new_order& incoming, rt::types::quantity& remaining)
{
  while (remaining > 0 && book.best_ask().has_value()) {
    const rt::types::price resting_ask = *book.best_ask();
    if (!taker_crosses_ask(incoming, resting_ask)) {
      break;
    }

    const order_state& maker = book.top_ask_front();
    const rt::types::quantity fill = std::min(remaining, maker.remaining_quantity);

    on_event(
      md::trade{
        .buy_user = md::types::user_id{incoming.user},
        .buy_order = md::types::user_order_id{incoming.order_id},
        .sell_user = md::types::user_id{maker.user},
        .sell_order = md::types::user_order_id{maker.order_id},
        .trade_price = md::types::price{resting_ask},
        .trade_quantity = md::types::quantity{fill},
      });

    remaining -= fill;

    if (const auto consumed = book.fill_top_ask_front(fill); consumed.has_value()) {
      resting_index_.erase(*consumed);
    }
  }
}

void engine::match_sell(order_book& book, const rt::new_order& incoming, rt::types::quantity& remaining)
{
  while (remaining > 0 && book.best_bid().has_value()) {
    const rt::types::price resting_bid = *book.best_bid();
    if (!taker_crosses_bid(incoming, resting_bid)) {
      break;
    }

    const order_state& maker = book.top_bid_front();
    const rt::types::quantity fill = std::min(remaining, maker.remaining_quantity);

    on_event(
      md::trade{
        .buy_user = md::types::user_id{maker.user},
        .buy_order = md::types::user_order_id{maker.order_id},
        .sell_user = md::types::user_id{incoming.user},
        .sell_order = md::types::user_order_id{incoming.order_id},
        .trade_price = md::types::price{resting_bid},
        .trade_quantity = md::types::quantity{fill},
      });

    remaining -= fill;

    if (const auto consumed = book.fill_top_bid_front(fill); consumed.has_value()) {
      resting_index_.erase(*consumed);
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

} // namespace matching_engine::v2
