#include "matching_engine/v2/order_book.hpp"

#include "lab/assert.hpp"

#include <cstdint>
#include <new>

namespace rt = order_routing;

namespace matching_engine::v2 {

order_book::order_book(boost::pool<>& node_pool)
  : pool_{node_pool}
{
}

order_book::~order_book()
{
  // Unlink and free every node before the side maps tear down, so each
  // borrowed slot goes back to the engine-owned pool cleanly.
  for (auto& level_entry : bids_) {
    release_level(level_entry.second);
  }
  for (auto& level_entry : asks_) {
    release_level(level_entry.second);
  }
}

order_node* order_book::allocate_node(const order_state& data)
{
  void* memory = pool_.malloc();
  if (memory == nullptr) {
    throw std::bad_alloc{};
  }
  auto* node = new (memory) order_node;
  node->data = data;
  return node;
}

void order_book::release_node(order_node* node)
{
  node->~order_node();
  pool_.free(node);
}

void order_book::release_level(level_list& level)
{
  while (!level.empty()) {
    auto* node = &level.front();
    level.pop_front();
    release_node(node);
  }
}

order_node* order_book::place(const order_state& resting)
{
  auto* node = allocate_node(resting);

  switch (resting.order_side) {
    case rt::types::side::buy:
      bids_[resting.limit_price].push_back(*node);
      break;

    case rt::types::side::sell:
      asks_[resting.limit_price].push_back(*node);
      break;
  }

  return node;
}

void order_book::cancel(order_node* handle)
{
  LAB_ASSERT(handle != nullptr);

  const order_state& resting = handle->data;

  switch (resting.order_side) {
    case rt::types::side::buy: {
      const auto level_it = bids_.find(resting.limit_price);
      LAB_ASSERT(level_it != bids_.end());
      auto& level = level_it->second;
      level.erase(level.iterator_to(*handle));
      if (level.empty()) {
        bids_.erase(level_it);
      }
      break;
    }

    case rt::types::side::sell: {
      const auto level_it = asks_.find(resting.limit_price);
      LAB_ASSERT(level_it != asks_.end());
      auto& level = level_it->second;
      level.erase(level.iterator_to(*handle));
      if (level.empty()) {
        asks_.erase(level_it);
      }
      break;
    }
  }

  release_node(handle);
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
  for (const auto& node : bids_.begin()->second) {
    sum += node.data.remaining_quantity;
  }
  return sum;
}

std::optional<rt::types::quantity> order_book::total_at_best_ask() const
{
  if (asks_.empty()) {
    return std::nullopt;
  }

  rt::types::quantity sum{0};
  for (const auto& node : asks_.begin()->second) {
    sum += node.data.remaining_quantity;
  }
  return sum;
}

const order_state& order_book::top_bid_front() const
{
  LAB_ASSERT(!bids_.empty());
  const auto& level = bids_.begin()->second;
  LAB_ASSERT(!level.empty());
  return level.front().data;
}

const order_state& order_book::top_ask_front() const
{
  LAB_ASSERT(!asks_.empty());
  const auto& level = asks_.begin()->second;
  LAB_ASSERT(!level.empty());
  return level.front().data;
}

std::optional<types::order_key> order_book::fill_top_bid_front(rt::types::quantity fill)
{
  LAB_ASSERT(!bids_.empty());
  const auto level_it = bids_.begin();
  auto& level = level_it->second;
  LAB_ASSERT(!level.empty());

  auto& node = level.front();
  LAB_ASSERT(fill <= node.data.remaining_quantity);
  node.data.remaining_quantity -= fill;

  if (node.data.remaining_quantity != 0) {
    return std::nullopt;
  }

  const types::order_key consumed{.user = node.data.user, .order_id = node.data.order_id};
  level.pop_front();
  release_node(&node);
  if (level.empty()) {
    bids_.erase(level_it);
  }
  return consumed;
}

std::optional<types::order_key> order_book::fill_top_ask_front(rt::types::quantity fill)
{
  LAB_ASSERT(!asks_.empty());
  const auto level_it = asks_.begin();
  auto& level = level_it->second;
  LAB_ASSERT(!level.empty());

  auto& node = level.front();
  LAB_ASSERT(fill <= node.data.remaining_quantity);
  node.data.remaining_quantity -= fill;

  if (node.data.remaining_quantity != 0) {
    return std::nullopt;
  }

  const types::order_key consumed{.user = node.data.user, .order_id = node.data.order_id};
  level.pop_front();
  release_node(&node);
  if (level.empty()) {
    asks_.erase(level_it);
  }
  return consumed;
}

} // namespace matching_engine::v2
