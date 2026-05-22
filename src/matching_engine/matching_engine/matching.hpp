#ifndef MATCHING_ENGINE_MATCHING_HPP
#define MATCHING_ENGINE_MATCHING_HPP

#include "market_data/messages.hpp"
#include "market_data/types.hpp"
#include "matching_engine/order_book.hpp"
#include "matching_engine/order_node.hpp"
#include "matching_engine/order_state.hpp"
#include "order_entry/types.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>

/*
 * Side-agnostic matching algorithm: taker + side map + cross
 * predicate + (on_trade, on_release) callbacks. Stateless and free of
 * engine state so unit tests drive it against a hand-rolled side map.
 *
 * Callers pick the map to walk (asks for buy takers, bids for sell).
 * make_trade does buyer/seller role assignment.
 */

namespace matching_engine {

/* `last_traded` is nullopt iff trades == 0. */
struct execution_summary
{
  std::size_t trades = 0;
  std::optional<order_entry::types::price> last_traded;
};

/*
 * Market orders carry price == 0 (the IOC wire contract) and
 * cross unconditionally.
 */
inline bool crosses(const order_state& taker, order_entry::types::price level_price)
{
  if (taker.price == 0) {
    return true;
  }

  switch (taker.side) {
    case order_entry::types::side::buy:
      return taker.price >= level_price;

    case order_entry::types::side::sell:
      return taker.price <= level_price;
  }

  std::terminate();
}

/*
 * The wire payload reports buyer and seller as separate fields regardless
 * of which side initiated the cross; this maps (taker, maker) to those
 * roles by inspecting the taker's side.
 */
inline market_data::trade make_trade(
  const order_state& taker,
  const order_state& maker,
  order_entry::types::price level_price,
  order_entry::types::quantity fill_qty)
{
  const order_state& buyer = (taker.side == order_entry::types::side::buy) ? taker : maker;
  const order_state& seller = (taker.side == order_entry::types::side::buy) ? maker : taker;

  return market_data::trade{
    .security_id = market_data::types::security_id{taker.security_id.get()},
    .trade_id = market_data::types::trade_id{0},
    .price = market_data::types::price{level_price},
    .quantity = market_data::types::quantity{fill_qty},
    .buyer = market_data::types::order_id{buyer.cl_ord_id.get()},
    .seller = market_data::types::order_id{seller.cl_ord_id.get()},
    .trade_condition = market_data::types::trade_condition::regular,
    .trade_sub_type = std::nullopt,
    .trade_date = market_data::types::trade_date{"20260522"},
    .transact_time = market_data::types::timestamp{0},
  };
}

template <typename OnTrade, typename OnRelease>
std::size_t consume_level(
  price_level& level, order_entry::types::price level_price, order_state& taker, OnTrade on_trade, OnRelease on_release)
{
  std::size_t trades = 0;

  // face the FIFO head as the next maker while taker quantity remains.
  while (!level.orders.empty() && taker.leaves_qty > 0) {
    auto& maker_node = level.orders.front();
    auto& maker = maker_node.data;

    // fill the smaller of the two; the larger side keeps the residual.
    const order_entry::types::quantity fill_qty = std::min(taker.leaves_qty, maker.leaves_qty);

    on_trade(make_trade(taker, maker, level_price, fill_qty));

    // decrement both sides and the level's running total in lockstep.
    taker.leaves_qty -= fill_qty;
    maker.leaves_qty -= fill_qty;
    level.total_remaining -= fill_qty;
    ++trades;

    // a fully-filled maker leaves the level immediately. A partial fill on
    // the maker means the taker is exhausted, so the loop will exit on its
    // next condition check.
    if (maker.leaves_qty == 0) {
      level.orders.pop_front();
      on_release(&maker_node);
    }
  }

  return trades;
}

/*
 * Walks `side_map` from best outward, consuming each level the taker
 * crosses until quantity runs out or the next level no longer crosses.
 *
 * Consumed levels are erased in one bulk range erase after the loop, not
 * per iteration. flat_map per-element erase is O(N) (vector shift) while
 * range erase shifts the tail once; the loop body also stays purely
 * arithmetic without re-acquiring begin() and reasoning about iterator
 * invalidation on every step. After the loop, [begin, it) is exactly the
 * fully-consumed range in all three exit paths (end, stopped crossing,
 * partial fill).
 */
template <typename SideMap, typename CrossPredicate, typename OnTrade, typename OnRelease>
execution_summary match(SideMap& side_map, order_state& taker, CrossPredicate crosses, OnTrade on_trade, OnRelease on_release)
{
  execution_summary result;

  // walk the side map from best outward; consume each crossing level until
  // the taker quantity runs out or the next level no longer crosses.
  auto it = side_map.begin();
  while (it != side_map.end() && taker.leaves_qty > 0 && crosses(taker, it->first)) {
    const auto level_trades = consume_level(it->second, it->first, taker, on_trade, on_release);

    // track the last touched price so callers know what cleared this round.
    if (level_trades > 0) {
      result.last_traded = it->first;
      result.trades += level_trades;
    }

    // empty level => fully consumed, step on. Non-empty => taker ran out
    // mid-level, leave the partial maker in place and stop.
    if (it->second.orders.empty()) {
      ++it;
    } else {
      break;
    }
  }

  // bulk-erase the consumed prefix in one shot (see header note above for
  // the flat_map cost argument).
  if (result.trades > 0) {
    side_map.erase(side_map.begin(), it);
  }

  return result;
}

} // namespace matching_engine

#endif /* MATCHING_ENGINE_MATCHING_HPP */
