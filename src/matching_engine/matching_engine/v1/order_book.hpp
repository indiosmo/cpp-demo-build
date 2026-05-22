#ifndef MATCHING_ENGINE_V1_ORDER_BOOK_HPP
#define MATCHING_ENGINE_V1_ORDER_BOOK_HPP

#include "matching_engine/order_state.hpp"
#include "order_entry/types.hpp"

#include "lab/result.hpp"

#include <functional>
#include <map>
#include <optional>
#include <vector>

/*
 * Per-symbol order book, v1: price-keyed std::map (desc bids, asc
 * asks), each level a std::vector<order_state> in arrival order. Simplest
 * shape with the right semantics; comparison in ADR 0004.
 */

namespace matching_engine::v1 {

class order_book
{
public:
  using bids_map = std::map<order_entry::types::price, std::vector<order_state>, std::greater<>>;
  using asks_map = std::map<order_entry::types::price, std::vector<order_state>, std::less<>>;

  order_book() = default;

  void place(const order_state&);

  /* Returns an error when no matching resting order exists. */
  lab::result<order_state> cancel(order_entry::types::client_id, order_entry::types::cl_ord_id);

  [[nodiscard]] std::optional<order_entry::types::price> best_bid() const;
  [[nodiscard]] std::optional<order_entry::types::price> best_ask() const;
  [[nodiscard]] std::optional<order_entry::types::quantity> total_at_best_bid() const;
  [[nodiscard]] std::optional<order_entry::types::quantity> total_at_best_ask() const;

  bids_map& bids()
  {
    return bids_;
  }

  asks_map& asks()
  {
    return asks_;
  }

  [[nodiscard]] const bids_map& bids() const
  {
    return bids_;
  }

  [[nodiscard]] const asks_map& asks() const
  {
    return asks_;
  }

private:
  bids_map bids_;
  asks_map asks_;
};

} // namespace matching_engine::v1

#endif /* MATCHING_ENGINE_V1_ORDER_BOOK_HPP */
