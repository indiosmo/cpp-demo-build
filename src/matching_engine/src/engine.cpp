#include "matching_engine/engine.hpp"

#include "boost/leaf/handle_errors.hpp"
#include "matching_engine/conversions.hpp"
#include "matching_engine/error_code.hpp"
#include "matching_engine/errors.hpp"
#include "matching_engine/factories.hpp"
#include "matching_engine/order_state.hpp"
#include "matching_engine/matching.hpp"

#include "lab/assert.hpp"
#include "lab/error.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"
#include "lab/variant.hpp"

#include <new>
#include <optional>
#include <string>
#include <utility>

namespace matching_engine {

namespace md = market_data;
namespace rt = order_entry;

engine::engine(engine_config configuration)
  : node_pool_{sizeof(order_node), configuration.node_pool_chunk_size}
{
  // pre-warm the pool to the expected resident count so the first burst
  // of new_order_singles does not pay a system allocation.
  reserve_node_pool(configuration.expected_resting_orders);

  // open one book per configured symbol; reserve both maps so the
  // steady-state path never rehashes.
  books_.reserve(configuration.valid_symbols.size());
  for (const auto& symbol : configuration.valid_symbols) {
    books_.try_emplace(symbol);
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
execution_summary engine::match_orders(order_book& book, order_state& taker)
{
  // wire engine callbacks so the side-agnostic algorithm can publish trades
  // and return fully-consumed maker slots to the pool.
  const auto trade_cb = [this](const md::trade& trade_event) { on_trade(trade_event); };
  const auto release_cb = [this](order_node* consumed) { on_release(consumed); };

  // a buy taker eats the ask side; a sell taker eats the bid side.
  switch (taker.side) {
    case rt::types::side::buy:
      return match(book.asks(), taker, crosses, trade_cb, release_cb);

    case rt::types::side::sell:
      return match(book.bids(), taker, crosses, trade_cb, release_cb);
  }

  std::terminate();
}

void engine::handle(const rt::new_order_single& request)
{
  boost::leaf::try_handle_all(
    [&] { return handle_new_order_single_impl(request); },
    [&](lab::match_error<errors::duplicate_order>) {
      LAB_LOG_WARN(
        "rejecting new_order_single: duplicate (client_id={}, cl_ord_id={}) already resting", request.client_id, request.cl_ord_id);
    },
    [&](lab::match_error<errors::unknown_symbol>) {
      LAB_LOG_WARN("rejecting new_order_single: unknown symbol {}", request.symbol);
    },
    [](const lab::error& err) { LAB_LOG_ERROR("unhandled engine error: {}", err.full_details()); },
    [] { LAB_LOG_ERROR("unhandled engine error: unknown failure"); });
}

lab::result<void> engine::handle_new_order_single_impl(const rt::new_order_single& request)
{
  const types::order_key incoming_key{.client_id = request.client_id, .cl_ord_id = request.cl_ord_id};

  // reject duplicates and resolve the destination book up-front.
  BOOST_LEAF_CHECK(check_duplicate(incoming_key));
  BOOST_LEAF_ASSIGN(auto* book_ptr, find_book(request.symbol));
  auto& book = *book_ptr;

  // IMPROVEMENT: event callbacks run inside the leaf-handled body, so subscriber
  // exceptions cross the leaf frame mid-emit. This should be redesigned to have
  // callbacks run outside the leaf context.

  // working copy; match_orders decrements leaves_qty in
  // place and any residual is what we rest below.
  order_state order = make_order_state(request);

  emit_execution_report(order, rt::types::exec_type::new_order, rt::types::ord_status::new_order);

  // cross the taker against the book; trades and releases fire via callbacks.
  const auto summary = match_orders(book, order);

  // classify the residual: market remainders (price == 0 on the wire,
  // IOC contract) are dropped, limit remainders rest.
  const bool removed_liquidity = summary.trades > 0;
  const bool is_market = order.price == 0;
  const bool should_place_order = order.leaves_qty > 0 && !is_market;
  const auto filled_qty = rt::types::quantity{request.order_qty.get() - order.leaves_qty.get()};

  if (removed_liquidity && summary.last_traded.has_value()) {
    on_market_data(
      md::execution_summary{
        .security_id = md::types::security_id{request.security_id.get()},
        .aggressor_side = to_market_side(request.side),
        .last_px = md::types::price{summary.last_traded->get()},
        .fill_qty = md::types::quantity{filled_qty.get()},
        .traded_hidden_qty = std::nullopt,
        .cancel_qty = std::nullopt,
        .aggressor_time = md::types::timestamp{0},
        .transact_time = md::types::timestamp{0},
      });

    const auto status = order.leaves_qty == 0 ? rt::types::ord_status::filled : rt::types::ord_status::partially_filled;
    emit_execution_report(
      order,
      rt::types::exec_type::trade,
      status,
      std::nullopt,
      filled_qty,
      rt::types::price{summary.last_traded->get()});
  }

  if (should_place_order) {
    // rest the residual: pool-allocate a node, link it into the book,
    // register its identity for future cancels and duplicate checks.
    // IMPROVEMENT: if any step is ever made fallible, wrap the sequence
    // in a lab::scope_guard-backed helper so partial progress rolls back.
    order_node* node = allocate_node(order);
    book.place(node);
    resting_index_.emplace(incoming_key, node);
  }

  // mbo_book_update emission. Post-trade always reports the consumed side
  // (opposite of the taker); own-side follows only if the residual landed
  // at the new best. See docs/engine-specs.md.
  if (removed_liquidity) {
    emit_mbo_book_update(book, rt::types::opposite(request.side), request.security_id);
  }

  if (should_place_order) {
    const auto own_best = (request.side == rt::types::side::buy) ? book.best_bid() : book.best_ask();
    if (own_best == request.price) {
      emit_mbo_book_update(book, request.side, request.security_id);
    }
  }

  return {};
}

lab::result<void> engine::check_duplicate(const types::order_key& key) const
{
  // (client_id, cl_ord_id) is the cross-symbol identity for a resting order;
  // a hit in the index means a duplicate that we must reject.
  if (resting_index_.contains(key)) {
    return lab::make_leaf_error(errors::duplicate_order{.client_id = key.client_id, .cl_ord_id = key.cl_ord_id});
  }
  return {};
}

lab::result<order_book*> engine::find_book(rt::types::symbol symbol)
{
  // engine_config preallocates one book per configured symbol; an unknown
  // symbol is an operator misconfiguration, not a wire event.
  const auto book_it = books_.find(symbol);
  if (book_it == books_.end()) {
    return lab::make_leaf_error(errors::unknown_symbol{.symbol = symbol});
  }

  return &book_it->second;
}

void engine::on_trade(md::trade trade_event)
{
  trade_event.trade_id = md::types::trade_id{next_trade_id_++};
  on_market_data(trade_event);
}

void engine::on_release(order_node* consumed)
{
  // fully-filled maker: drop its identity index entry and return the slot to the pool.
  resting_index_.erase(types::order_key{.client_id = consumed->data.client_id, .cl_ord_id = consumed->data.cl_ord_id});
  release_node(consumed);
}

void engine::handle(const rt::cancel_order& incoming)
{
  const types::order_key key{.client_id = incoming.client_id, .cl_ord_id = rt::types::cl_ord_id{incoming.orig_cl_ord_id.get()}};

  // find the resting order by identity. a miss means it was already filled,
  // canceled, or never placed -- drop silently.
  const auto index_it = resting_index_.find(key);
  if (index_it == resting_index_.end()) {
    LAB_LOG_WARN("cancel miss: client_id={} orig_cl_ord_id={}", incoming.client_id, incoming.orig_cl_ord_id);
    emit_cancel_reject(incoming, rt::types::reject_reason::unknown_order, "unknown order");
    return;
  }

  // resolve the book the order is resting in. the engine placed it there,
  // so the entry must exist -- a missing book here is a state-corruption bug.
  order_node* node = index_it->second;
  const order_state resting = node->data;

  const auto book_it = books_.find(resting.symbol);
  LAB_ASSERT(book_it != books_.end());
  auto& book = book_it->second;

  // capture book-update impact before the cancel removes the level.
  // Only cancels at the current best level produce a book update.
  const bool affects_mbo_book_update = (book.best(resting.side) == resting.price);

  // unlink from the book, drop the identity entry, return the slot to the pool.
  book.cancel(node);
  resting_index_.erase(index_it);
  release_node(node);
  emit_execution_report(
    resting,
    rt::types::exec_type::canceled,
    rt::types::ord_status::canceled,
    incoming.orig_cl_ord_id);

  if (affects_mbo_book_update) {
    emit_mbo_book_update(book, resting.side, resting.security_id);
  }
}

void engine::handle(const rt::replace_order& incoming)
{
  const types::order_key old_key{.client_id = incoming.client_id, .cl_ord_id = rt::types::cl_ord_id{incoming.orig_cl_ord_id.get()}};
  const auto index_it = resting_index_.find(old_key);
  if (index_it == resting_index_.end()) {
    emit_cancel_reject(
      rt::cancel_order{.client_id = incoming.client_id, .cl_ord_id = incoming.cl_ord_id, .orig_cl_ord_id = incoming.orig_cl_ord_id},
      rt::types::reject_reason::unknown_order,
      "unknown order");
    return;
  }

  const types::order_key new_key{.client_id = incoming.client_id, .cl_ord_id = incoming.cl_ord_id};
  if (resting_index_.contains(new_key)) {
    emit_cancel_reject(
      rt::cancel_order{.client_id = incoming.client_id, .cl_ord_id = incoming.cl_ord_id, .orig_cl_ord_id = incoming.orig_cl_ord_id},
      rt::types::reject_reason::duplicate_order,
      "duplicate cl_ord_id");
    return;
  }

  order_node* node = index_it->second;
  const order_state resting = node->data;
  const auto book_it = books_.find(resting.symbol);
  LAB_ASSERT(book_it != books_.end());
  auto& book = book_it->second;
  const bool affects_mbo_book_update = (book.best(resting.side) == resting.price);

  book.cancel(node);
  resting_index_.erase(index_it);
  release_node(node);

  if (affects_mbo_book_update) {
    emit_mbo_book_update(book, resting.side, resting.security_id);
  }

  rt::new_order_single replacement{
    .client_id = incoming.client_id,
    .cl_ord_id = incoming.cl_ord_id,
    .security_id = incoming.security_id,
    .symbol = incoming.symbol,
    .security_exchange = incoming.security_exchange,
    .side = incoming.side,
    .ord_type = incoming.ord_type,
    .time_in_force = incoming.time_in_force,
    .order_qty = incoming.order_qty,
    .price = incoming.price,
  };

  boost::leaf::try_handle_all(
    [&] { return handle_new_order_single_impl(replacement); },
    [&](lab::match_error<errors::duplicate_order>) {
      emit_cancel_reject(
        rt::cancel_order{.client_id = incoming.client_id, .cl_ord_id = incoming.cl_ord_id, .orig_cl_ord_id = incoming.orig_cl_ord_id},
        rt::types::reject_reason::duplicate_order,
        "duplicate cl_ord_id");
    },
    [&](lab::match_error<errors::unknown_symbol>) {
      emit_cancel_reject(
        rt::cancel_order{.client_id = incoming.client_id, .cl_ord_id = incoming.cl_ord_id, .orig_cl_ord_id = incoming.orig_cl_ord_id},
        rt::types::reject_reason::unknown_symbol,
        "unknown symbol");
    },
    [](const lab::error& err) { LAB_LOG_ERROR("unhandled engine error: {}", err.full_details()); },
    [] { LAB_LOG_ERROR("unhandled engine error: unknown failure"); });
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
        resting_index_.erase(types::order_key{.client_id = node.data.client_id, .cl_ord_id = node.data.cl_ord_id});
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

void engine::emit_execution_report(
  const order_state& order,
  rt::types::exec_type exec_type,
  rt::types::ord_status ord_status,
  std::optional<rt::types::orig_cl_ord_id> orig_cl_ord_id,
  std::optional<rt::types::quantity> last_qty,
  std::optional<rt::types::price> last_px)
{
  const auto cum_qty = rt::types::quantity{order.order_qty.get() - order.leaves_qty.get()};

  on_order_entry(
    rt::execution_report{
      .client_id = order.client_id,
      .cl_ord_id = order.cl_ord_id,
      .orig_cl_ord_id = orig_cl_ord_id,
      .order_id = rt::types::order_id{order.cl_ord_id.get()},
      .exec_id = rt::types::exec_id{next_exec_id_++},
      .security_id = order.security_id,
      .symbol = order.symbol,
      .security_exchange = order.security_exchange,
      .side = order.side,
      .ord_type = order.ord_type,
      .time_in_force = order.time_in_force,
      .exec_type = exec_type,
      .ord_status = ord_status,
      .order_qty = order.order_qty,
      .cum_qty = cum_qty,
      .leaves_qty = order.leaves_qty,
      .last_qty = last_qty,
      .last_px = last_px,
      .avg_px = last_px,
      .transact_time = rt::types::timestamp{0},
      .reject_reason = std::nullopt,
      .text = {},
    });
}

void engine::emit_cancel_reject(const rt::cancel_order& cancel, rt::types::reject_reason reject_reason, std::string text)
{
  on_order_entry(
    rt::cancel_reject{
      .client_id = cancel.client_id,
      .cl_ord_id = cancel.cl_ord_id,
      .orig_cl_ord_id = cancel.orig_cl_ord_id,
      .reject_reason = reject_reason,
      .text = std::move(text),
      .transact_time = rt::types::timestamp{0},
    });
}

void engine::emit_mbo_book_update(const order_book& book, rt::types::side affected_side, rt::types::security_id security_id)
{
  const auto snapshot = book.best_level(affected_side);

  std::optional<md::types::price> outbound_top_price;
  std::optional<md::types::quantity> outbound_top_quantity;
  if (snapshot.has_value()) {
    outbound_top_price = md::types::price{snapshot->price};
    outbound_top_quantity = md::types::quantity{snapshot->total_quantity};
  }

  on_market_data(
    md::mbo_book_update{
      .security_id = md::types::security_id{security_id.get()},
      .update_action = snapshot.has_value() ? md::types::update_action::change : md::types::update_action::delete_order,
      .side = to_market_side(affected_side),
      .resting_order_id = md::types::order_id{0},
      .price = outbound_top_price,
      .quantity = outbound_top_quantity,
      .previous_quantity = std::nullopt,
      .transact_time = md::types::timestamp{0},
    });
}

} // namespace matching_engine
