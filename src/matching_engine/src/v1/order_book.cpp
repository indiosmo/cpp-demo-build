#include "matching_engine/v1/order_book.hpp"

#include "order_entry/errors.hpp"

#include "lab/error.hpp"

#include <algorithm>

namespace rt = order_entry;

namespace matching_engine::v1 {

void order_book::place(const order_state& resting)
{
  switch (resting.side) {
    case rt::types::side::buy:
      bids_[resting.price].push_back(resting);
      break;

    case rt::types::side::sell:
      asks_[resting.price].push_back(resting);
      break;
  }
}

lab::result<order_state> order_book::cancel(rt::types::client_id client_id, rt::types::cl_ord_id cl_ord_id)
{
  const auto matches = [&](const order_state& candidate) { return candidate.client_id == client_id && candidate.cl_ord_id == cl_ord_id; };

  for (auto map_it = bids_.begin(); map_it != bids_.end(); ++map_it) {
    auto& level = map_it->second;
    auto level_it = std::ranges::find_if(level, matches);
    if (level_it == level.end()) {
      continue;
    }

    order_state removed = *level_it;
    level.erase(level_it);
    if (level.empty()) {
      bids_.erase(map_it);
    }
    return removed;
  }

  for (auto map_it = asks_.begin(); map_it != asks_.end(); ++map_it) {
    auto& level = map_it->second;
    auto level_it = std::ranges::find_if(level, matches);
    if (level_it == level.end()) {
      continue;
    }

    order_state removed = *level_it;
    level.erase(level_it);
    if (level.empty()) {
      asks_.erase(map_it);
    }
    return removed;
  }

  return lab::make_leaf_error(rt::errors::unknown_order{.client_id = client_id, .cl_ord_id = cl_ord_id});
}

std::optional<rt::types::price> order_book::best_bid() const
{
  if (bids_.empty()) {
    return std::nullopt;
  }

  return bids_.begin()->first;
}

std::optional<rt::types::price> order_book::best_ask() const
{
  if (asks_.empty()) {
    return std::nullopt;
  }

  return asks_.begin()->first;
}

std::optional<rt::types::quantity> order_book::total_at_best_bid() const
{
  if (bids_.empty()) {
    return std::nullopt;
  }

  rt::types::quantity sum{0};
  for (const auto& resting : bids_.begin()->second) {
    sum += resting.leaves_qty;
  }
  return sum;
}

std::optional<rt::types::quantity> order_book::total_at_best_ask() const
{
  if (asks_.empty()) {
    return std::nullopt;
  }

  rt::types::quantity sum{0};
  for (const auto& resting : asks_.begin()->second) {
    sum += resting.leaves_qty;
  }
  return sum;
}

} // namespace matching_engine::v1
