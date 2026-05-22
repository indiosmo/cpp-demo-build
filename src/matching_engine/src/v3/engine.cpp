#include "matching_engine/v3/engine.hpp"

#include "boost/leaf/handle_errors.hpp"
#include "matching_engine/conversions.hpp"
#include "matching_engine/error_code.hpp"
#include "matching_engine/errors.hpp"
#include "matching_engine/factories.hpp"
#include "matching_engine/order_state.hpp"
#include "matching_engine/v3/matching.hpp"

#include "lab/assert.hpp"
#include "lab/error.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"
#include "lab/variant.hpp"

#include <new>
#include <optional>

namespace matching_engine::v3 {

namespace md = market_data;
namespace rt = order_routing;

engine::engine(engine_config configuration)
  : node_pool_{sizeof(order_node), configuration.node_pool_chunk_size}
{
  // pre-warm the pool to the expected resident count so the first burst
  // of new_orders does not pay a system allocation.
  reserve_node_pool(configuration.expected_resting_orders);

  // open one book per configured symbol; reserve both maps so the
  // steady-state path never rehashes.
  books_.reserve(configuration.valid_symbols.size());
  for (const auto& instrument : configuration.valid_symbols) {
    books_.try_emplace(instrument);
  }

  resting_index_.reserve(configuration.expected_resting_orders);
}

void engine::reserve_node_pool(std::size_t expected_resting_orders)
{
  if (expected_resting_orders == 0) {
    return;
  }

  // boost::pool grows lazily. Make the next system allocation large enough
  // for the expected resident count, force it with one malloc/free round
  // trip, then restore the configured chunk size for any later growth beyond
  // the startup hint.
  const auto next_chunk_size = node_pool_.get_next_size();
  node_pool_.set_next_size(expected_resting_orders);

  void* memory = node_pool_.malloc();
  if (memory == nullptr) {
    throw std::bad_alloc{};
  }

  node_pool_.free(memory);
  node_pool_.set_next_size(next_chunk_size);
}

void engine::send(const rt::request& cmd)
{
  lab::match(cmd, [this](const auto& cmd) { handle(cmd); });
}

order_node* engine::allocate_node(const order_state& data)
{
  // fast path after startup warmup: pop a raw slot from boost::pool's free list.
  void* memory = node_pool_.malloc();

  if (memory == nullptr) {
    throw std::bad_alloc{};
  }

  // construct the order_node in place over the pool slot, then copy in the payload.
  auto* node = new (memory) order_node;
  node->data = data;
  return node;
}

void engine::release_node(order_node* node)
{
  // The intrusive list only unlinks; the engine owns construction/destruction.
  node->~order_node();
  node_pool_.free(node);
}

// Side arrives as data here; a type-level side would let this dispatch become
// a direct call to match.
execution_summary engine::match_orders(flat_order_book& book, order_state& taker)
{
  // wire engine callbacks so the side-agnostic algorithm can publish trades
  // and return fully-consumed maker slots to the pool.
  const auto trade_cb = [this](const md::trade& trade_event) { on_trade(trade_event); };
  const auto release_cb = [this](order_node* consumed) { on_release(consumed); };

  // a buy taker eats the ask side; a sell taker eats the bid side.
  switch (taker.order_side) {
    case rt::types::side::buy:
      return match(book.asks(), taker, crosses, trade_cb, release_cb);

    case rt::types::side::sell:
      return match(book.bids(), taker, crosses, trade_cb, release_cb);
  }

  std::terminate();
}

void engine::handle(const rt::new_order& request)
{
  boost::leaf::try_handle_all(
    [&] { return handle_new_order_impl(request); },
    [&](lab::match_error<errors::duplicate_order>) {
      LAB_LOG_WARN(
        "rejecting new_order: duplicate (user={}, user_order_id={}) already resting", request.user, request.order_id);
    },
    [&](lab::match_error<errors::unknown_symbol>) {
      LAB_LOG_WARN("rejecting new_order: unknown symbol {}", request.instrument);
    },
    [](const lab::error& err) { LAB_LOG_ERROR("unhandled engine error: {}", err.full_details()); },
    [] { LAB_LOG_ERROR("unhandled engine error: unknown failure"); });
}

lab::result<void> engine::handle_new_order_impl(const rt::new_order& request)
{
  const types::order_key incoming_key{.user = request.user, .order_id = request.order_id};

  // reject duplicates and resolve the destination book up-front.
  BOOST_LEAF_CHECK(check_duplicate(incoming_key));
  BOOST_LEAF_ASSIGN(auto* book_ptr, find_book(request.instrument));
  auto& book = *book_ptr;

  // IMPROVEMENT: event callbacks run inside the leaf-handled body, so subscriber
  // exceptions cross the leaf frame mid-emit. This should be redesigned to have
  // callbacks run outside the leaf context.

  // ack precedes any trade so receivers see the order id before fills on it.
  on_event(
    md::order_ack{
      .user = md::types::user_id{request.user},
      .order_id = md::types::user_order_id{request.order_id},
    });

  // working copy; match_orders decrements remaining_quantity in
  // place and any residual is what we rest below.
  order_state order = make_order_state(request);

  // cross the taker against the book; trades and releases fire via callbacks.
  const auto summary = match_orders(book, order);

  // classify the residual: market remainders (limit_price == 0 on the wire,
  // IOC contract) are dropped, limit remainders rest.
  const bool removed_liquidity = summary.trades > 0;
  const bool is_market = order.limit_price == 0;
  const bool should_place_order = order.remaining_quantity > 0 && !is_market;

  if (should_place_order) {
    // rest the residual: pool-allocate a node, link it into the book,
    // register its identity for future cancels and duplicate checks.
    // IMPROVEMENT: if any step is ever made fallible, wrap the sequence
    // in a lab::scope_guard-backed helper so partial progress rolls back.
    order_node* node = allocate_node(order);
    book.place(node);
    resting_index_.emplace(incoming_key, node);
  }

  // top_of_book emission. Post-trade always reports the consumed side
  // (opposite of the taker); own-side follows only if the residual landed
  // at the new best. See docs/engine-specs.md.
  if (removed_liquidity) {
    emit_top_of_book(book, rt::types::opposite(request.order_side));
  }

  if (should_place_order) {
    const auto own_best = (request.order_side == rt::types::side::buy) ? book.best_bid() : book.best_ask();
    if (own_best == request.limit_price) {
      emit_top_of_book(book, request.order_side);
    }
  }

  return {};
}

lab::result<void> engine::check_duplicate(const types::order_key& key) const
{
  // (user, user_order_id) is the cross-symbol identity for a resting order;
  // a hit in the index means a duplicate that we must reject.
  if (resting_index_.contains(key)) {
    return lab::make_leaf_error(errors::duplicate_order{.user = key.user, .order_id = key.order_id});
  }
  return {};
}

lab::result<flat_order_book*> engine::find_book(rt::types::symbol instrument)
{
  // engine_config preallocates one book per configured symbol; an unknown
  // symbol is an operator misconfiguration, not a wire event.
  const auto book_it = books_.find(instrument);
  if (book_it == books_.end()) {
    return lab::make_leaf_error(errors::unknown_symbol{.instrument = instrument});
  }

  return &book_it->second;
}

void engine::on_trade(const md::trade& trade_event)
{
  on_event(trade_event);
}

void engine::on_release(order_node* consumed)
{
  // fully-filled maker: drop its identity index entry and return the slot to the pool.
  resting_index_.erase(types::order_key{.user = consumed->data.user, .order_id = consumed->data.order_id});
  release_node(consumed);
}

void engine::handle(const rt::cancel_order& incoming)
{
  const types::order_key key{.user = incoming.user, .order_id = incoming.order_id};

  // find the resting order by identity. a miss means it was already filled,
  // canceled, or never placed -- drop silently.
  const auto index_it = resting_index_.find(key);
  if (index_it == resting_index_.end()) {
    LAB_LOG_WARN("cancel miss: user={} order_id={}", incoming.user, incoming.order_id);
    // IMPROVEMENT: no reject event in the spec; a production protocol would emit one here.
    return;
  }

  // resolve the book the order is resting in. the engine placed it there,
  // so the entry must exist -- a missing book here is a state-corruption bug.
  order_node* node = index_it->second;
  const order_state resting = node->data;

  const auto book_it = books_.find(resting.instrument);
  LAB_ASSERT(book_it != books_.end());
  auto& book = book_it->second;

  // ack the cancel before mutating the book so observers see the ack ahead
  // of any follow-on top_of_book.
  on_event(
    md::cancel_ack{
      .user = md::types::user_id{incoming.user},
      .order_id = md::types::user_order_id{incoming.order_id},
    });

  // capture top-of-book impact before the cancel removes the level.
  // Only cancels at the current best level produce a top_of_book.
  const bool affects_top_of_book = (book.best(resting.order_side) == resting.limit_price);

  // unlink from the book, drop the identity entry, return the slot to the pool.
  book.cancel(node);
  resting_index_.erase(index_it);
  release_node(node);

  if (affects_top_of_book) {
    emit_top_of_book(book, resting.order_side);
  }
}

void engine::handle(const rt::flush&)
{
  // Flush is silent (docs/engine-specs.md); books stay alive so later
  // commands keep finding them.
  for (auto& book_entry : books_) {
    auto& book = book_entry.second;

    // Walk one side front-to-back, releasing nodes and dropping levels as they empty.
    // IMPROVEMENT: likely better expressed as a custom algorithm.
    auto drain_side = [this](auto& side_map) {
      while (!side_map.empty()) {
        auto& level = side_map.begin()->second;
        auto& node = level.orders.front();

        // drop the identity entry, unlink the FIFO head, return the slot to the pool.
        resting_index_.erase(types::order_key{.user = node.data.user, .order_id = node.data.order_id});
        level.orders.pop_front();
        const bool level_now_empty = level.orders.empty();
        release_node(&node);

        // drop the level as soon as its FIFO empties so the next iteration steps on.
        if (level_now_empty) {
          side_map.erase(side_map.begin());
        }
      }
    };

    drain_side(book.bids());
    drain_side(book.asks());
  }
}

/*
 * Emits the affected side's top level. price and total_quantity are optional
 * so an empty side renders as a "no top" message rather than a synthetic zero.
 */
void engine::emit_top_of_book(const flat_order_book& book, rt::types::side affected_side)
{
  const auto snapshot = book.best_level(affected_side);

  std::optional<md::types::price> outbound_top_price;
  std::optional<md::types::total_quantity> outbound_top_quantity;
  if (snapshot.has_value()) {
    outbound_top_price = md::types::price{snapshot->price};
    outbound_top_quantity = md::types::total_quantity{snapshot->total_quantity};
  }

  on_event(
    md::top_of_book{
      .book_side = to_market_side(affected_side),
      .top_price = outbound_top_price,
      .top_quantity = outbound_top_quantity,
    });
}

} // namespace matching_engine::v3
