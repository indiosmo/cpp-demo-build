#include "market_data/messages.hpp"
#include "matching_engine/engine.hpp"
#include "matching_engine/engine_config.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <random>
#include <variant>
#include <vector>

/*
 * Component-level benchmarks for matching_engine::engine. These sit
 * one layer above the order_book microbenchmarks: every operation goes through
 * the engine's variant dispatch, the per-symbol book lookup, and the
 * cross-symbol resting_index_ map, plus any top_of_book emit the wire contract
 * triggers. The shape of the workload (single deep symbol, ~6000 resting
 * orders per side, orders unevenly distributed across price levels) mirrors a
 * snapshot of a busy book a real venue would carry.
 *
 * Five scenarios:
 *
 *   - BM_PlaceLimitNoFill: batch of K places of a non-marketable buy at the
 *     existing best bid. Each place takes the engine's full new_order path
 *     including the post-place top_of_book emit and the level walk that
 *     computes the new total quantity. Pause-cancels them between batches.
 *
 *   - BM_PlaceCancelRoundTrip: single place + single cancel pair per
 *     iteration, both timed. Models the quote-refresh shape of a market maker
 *     and avoids pause/resume noise entirely; reports the round-trip cost.
 *
 *   - BM_CancelAtTopOfBook: batch of K cancels of orders at the best bid
 *     level. Each cancel emits a top_of_book whose total walks the surviving
 *     orders at the level, so this exercises the level-walk hot path that
 *     does not run on deep cancels.
 *
 *   - BM_CancelDeepInBook: batch of K cancels of orders at the deepest bid
 *     level. The cancelled price never matches the best bid, so the engine
 *     skips the top_of_book emit and the cancel runs at its minimum cost.
 *
 *   - BM_CrossManyOrders: one marketable buy per iteration sized to consume
 *     ~200 resting asks across several top levels. Captures the consumed
 *     orders via on_event during the timed cross and re-places them in
 *     pause-time so subsequent iterations cross the same workload.
 */

namespace {

namespace md = market_data;
namespace me = matching_engine;
namespace rt = order_routing;

const rt::types::symbol bench_symbol{"BENCH"};

// Workload shape. Both sides carry the same number of levels and the same
// per-level depth distribution; bids descend from base_bid_price by one tick
// per level, asks ascend from base_ask_price.
constexpr std::size_t levels_per_side = 256;
constexpr std::uint64_t base_bid_price = 1000;
constexpr std::uint64_t base_ask_price = 1001;
constexpr std::uint64_t price_tick = 1;

constexpr double mean_orders_per_level = 24.0;
constexpr double stddev_orders_per_level = 8.0;
constexpr std::size_t min_orders_per_level = 1;
constexpr std::size_t max_orders_per_level = 60;

constexpr double mean_quantity = 50.0;
constexpr double stddev_quantity = 20.0;
constexpr std::uint64_t min_quantity = 1;
constexpr std::uint64_t max_quantity = 200;

constexpr std::uint64_t random_seed = 0xC0FFEEULL;

// Each seeded order owns a unique user_order_id; the user id cycles across a
// small pool to mimic many participants posting concurrently. Cycling rather
// than holding all order ids under one user id keeps the engine's
// resting_index_ keys spread across the hash space in a realistic shape.
constexpr std::uint64_t distinct_users = 128;

// Batch size for the cancel/place benchmarks. K=64 keeps each timed segment
// near 30us on the heavy workload, well above google-benchmark's pause/resume
// overhead so per-op timing is dominated by the operation itself rather than
// the surrounding instrumentation.
constexpr std::size_t batch_size = 64;

// Cross size for the marketable-order benchmark. Sized large enough to walk
// several top ask levels and many resting orders so the trade emit / level
// erase / resting_index_ erase loop is the dominant cost.
constexpr std::size_t cross_target_orders = 200;

// Identifier ranges. Seeded orders use ids starting at 1; benchmark-owned
// "fresh" orders that come and go each iteration use ids well above the
// seeded range so there is no possibility of collision with the engine's
// duplicate-rejection check.
constexpr std::uint64_t fresh_id_base = 1ULL << 40;
constexpr rt::types::user_id fresh_user{1};

struct seeded_order
{
  rt::types::user_id user;
  rt::types::user_order_id order_id;
  rt::types::side order_side;
  rt::types::price limit_price;
  rt::types::quantity order_quantity;
};

template <typename T>
T clamp_to(double value, T low, T high)
{
  if (value < static_cast<double>(low)) {
    return low;
  }
  if (value > static_cast<double>(high)) {
    return high;
  }
  return static_cast<T>(value);
}

/*
 * Builds the deterministic seed plan. The plan is the flat list of orders
 * that, when sent to the engine in order, materialises the canonical book the
 * benchmarks all start from. Determinism (fixed seed) is what lets the
 * benchmarks restore state between iterations by replaying exact trades.
 *
 * The bid side is emitted from best to worst (so the best level is filled
 * first and its FIFO front is the lowest-id order), and likewise for the ask
 * side. Order ids are globally unique and grow monotonically; users cycle
 * across a small pool.
 */
std::vector<seeded_order> build_seed_plan()
{
  std::mt19937_64 rng{random_seed};
  std::normal_distribution<double> per_level_distribution{mean_orders_per_level, stddev_orders_per_level};
  std::normal_distribution<double> quantity_distribution{mean_quantity, stddev_quantity};

  std::vector<seeded_order> plan;
  plan.reserve(static_cast<std::size_t>(2 * levels_per_side * mean_orders_per_level));

  std::uint64_t next_order_id = 1;

  auto emit_side = [&](rt::types::side order_side, std::uint64_t starting_price, bool descending) {
    for (std::size_t level_index = 0; level_index < levels_per_side; ++level_index) {
      const std::uint64_t price_offset = static_cast<std::uint64_t>(level_index) * price_tick;
      const std::uint64_t level_price = descending ? starting_price - price_offset : starting_price + price_offset;
      const std::size_t orders_at_level =
        clamp_to<std::size_t>(per_level_distribution(rng), min_orders_per_level, max_orders_per_level);

      for (std::size_t i = 0; i < orders_at_level; ++i) {
        const std::uint64_t order_quantity = clamp_to<std::uint64_t>(quantity_distribution(rng), min_quantity, max_quantity);
        plan.push_back(
          seeded_order{
            .user = rt::types::user_id{(next_order_id % distinct_users) + 1},
            .order_id = rt::types::user_order_id{next_order_id},
            .order_side = order_side,
            .limit_price = rt::types::price{level_price},
            .order_quantity = rt::types::quantity{order_quantity},
          });
        ++next_order_id;
      }
    }
  };

  emit_side(rt::types::side::buy, base_bid_price, /*descending*/ true);
  emit_side(rt::types::side::sell, base_ask_price, /*descending*/ false);

  return plan;
}

me::engine_config bench_config(std::size_t expected_resting_orders)
{
  return me::engine_config{
    .valid_symbols{bench_symbol},
    .expected_resting_orders = expected_resting_orders,
    // Modest chunk size; the engine resizes the pool as needed and the
    // benchmark's seed phase commits the working-set in one warmup pass.
    .node_pool_chunk_size = 1024,
  };
}

/*
 * Engine wrapper that drops every emitted event. Trade-capture benchmarks
 * override on_event after construction; the rest leave it as a noop so the
 * timed loop measures the engine itself rather than the cost of accumulating
 * events into a vector.
 */
struct bench_engine
{
  me::engine engine;

  bench_engine()
    : engine{bench_config(/*expected_resting_orders*/ 32 * 1024)}
  {
    engine.on_event = [](const md::message&) {};
  }

  void send(const rt::request& request)
  {
    engine.send(request);
  }
};

rt::new_order to_new_order(const seeded_order& seed)
{
  return rt::new_order{
    .user = seed.user,
    .order_id = seed.order_id,
    .instrument = bench_symbol,
    .order_side = seed.order_side,
    .limit_price = seed.limit_price,
    .order_quantity = seed.order_quantity,
  };
}

void seed(bench_engine& engine, const std::vector<seeded_order>& plan)
{
  for (const auto& entry : plan) {
    engine.send(to_new_order(entry));
  }
}

void BM_PlaceLimitNoFill(benchmark::State& state)
{
  // Place K=batch_size fresh non-marketable buys at the current best bid
  // (existing level, no cross). Each place hits the engine's full new_order
  // path: variant dispatch, duplicate check, book lookup, the book's
  // intrusive-list append, the resting_index_ insert, and the post-place
  // top_of_book emit (own_best == limit_price, so the engine emits and the
  // emit path walks the level's surviving orders).
  //
  // Cancels happen in pause-time so only the timed segment counts placement.
  // Fresh order ids occupy a high range that does not collide with seeded
  // ids; one user owns all of them, which is cheaper than rotating users but
  // does not change the placement cost the benchmark measures.
  const auto plan = build_seed_plan();
  bench_engine engine;
  seed(engine, plan);

  const rt::types::price target_price{base_bid_price};
  std::vector<rt::request> place_batch;
  std::vector<rt::request> cancel_batch;
  place_batch.reserve(batch_size);
  cancel_batch.reserve(batch_size);
  for (std::size_t i = 0; i < batch_size; ++i) {
    const rt::types::user_order_id fresh_id{fresh_id_base + i};
    place_batch.push_back(
      rt::new_order{
        .user = fresh_user,
        .order_id = fresh_id,
        .instrument = bench_symbol,
        .order_side = rt::types::side::buy,
        .limit_price = target_price,
        .order_quantity = rt::types::quantity{1},
      });
    cancel_batch.push_back(
      rt::cancel_order{
        .user = fresh_user,
        .order_id = fresh_id,
      });
  }

  for (auto _ : state) {
    for (const auto& command : place_batch) {
      engine.send(command);
    }

    for (const auto& command : cancel_batch) {
      engine.send(command);
    }
  }

  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) * static_cast<std::int64_t>(batch_size));
}

void BM_PlaceCancelRoundTrip(benchmark::State& state)
{
  // Quote-refresh shape: place a fresh limit at the best bid, then cancel it.
  // Both ops are timed; no pause/resume. The per-iteration measurement is the
  // round-trip cost, normalised below by SetItemsProcessed so the column reads
  // as one (place + cancel) pair per item.
  //
  // Reusing the same (user, order_id) across iterations is safe because each
  // iteration ends with the order cancelled and removed from resting_index_.
  const auto plan = build_seed_plan();
  bench_engine engine;
  seed(engine, plan);

  const rt::types::user_order_id roundtrip_id{fresh_id_base};
  const rt::request place_command = rt::new_order{
    .user = fresh_user,
    .order_id = roundtrip_id,
    .instrument = bench_symbol,
    .order_side = rt::types::side::buy,
    .limit_price = rt::types::price{base_bid_price},
    .order_quantity = rt::types::quantity{1},
  };
  const rt::request cancel_command = rt::cancel_order{
    .user = fresh_user,
    .order_id = roundtrip_id,
  };

  for (auto _ : state) {
    engine.send(place_command);
    engine.send(cancel_command);
  }

  state.SetItemsProcessed(state.iterations());
}

/*
 * Setup-driven fixtures for the cancel and cross benchmarks.
 *
 * The Google Benchmark project documents PauseTiming/ResumeTiming as a hot
 * source of measurement noise (hundreds of nanoseconds per pair) and points
 * users at the Setup/Teardown callbacks plus Iterations()/Repetitions() as
 * the preferred way to keep per-iteration work outside the timed region.
 *
 * Each fixture below pre-builds a vector of commands sized to the iteration
 * count; the timed loop walks the vector with a cursor and sends one command
 * per iteration. Setup is invoked once per Repetition by the framework, which
 * re-seeds the engine and rebuilds the command vector so each repetition
 * starts from the same workload shape. With per-iteration cost in the
 * 50-500 ns range and a few hundred iterations per repetition, the timed
 * segment dominates the cursor advance and a couple of variant constructions.
 */

// Number of cancel operations measured per repetition. Picked to match the
// original slab_depth - batch_size headroom (~64) so the level depth stays in
// the same regime as the prior benchmark (256..192 surviving orders during
// the run for the at-top fixture), keeping the top_of_book level walk a
// comparable workload.
constexpr std::size_t cancel_iterations_per_repetition = 64;

// Slab depth placed behind the cancel queue so the level never collapses
// during the timed run; matches the headroom the original used.
constexpr std::size_t cancel_slab_headroom = 192;

constexpr int cancel_repetitions = 30;

struct cancel_at_level_fixture
{
  std::optional<bench_engine> engine;
  std::vector<rt::request> cancels;
};

cancel_at_level_fixture g_cancel_top_fixture;
cancel_at_level_fixture g_cancel_deep_fixture;

void prepare_cancel_fixture(cancel_at_level_fixture& fixture, rt::types::side order_side, rt::types::price target_price)
{
  fixture.engine.emplace();
  seed(*fixture.engine, build_seed_plan());

  fixture.cancels.clear();
  fixture.cancels.reserve(cancel_iterations_per_repetition);

  std::uint64_t next_id = fresh_id_base;

  // Slab headroom lives in front of (older than) the cancellable orders so
  // the cancels target the back of the queue and the surviving headroom keeps
  // the level walk realistic throughout the run.
  for (std::size_t i = 0; i < cancel_slab_headroom; ++i) {
    const rt::types::user_order_id order_id{next_id++};
    fixture.engine->send(
      rt::new_order{
        .user = fresh_user,
        .order_id = order_id,
        .instrument = bench_symbol,
        .order_side = order_side,
        .limit_price = target_price,
        .order_quantity = rt::types::quantity{1},
      });
  }

  for (std::size_t i = 0; i < cancel_iterations_per_repetition; ++i) {
    const rt::types::user_order_id order_id{next_id++};
    fixture.engine->send(
      rt::new_order{
        .user = fresh_user,
        .order_id = order_id,
        .instrument = bench_symbol,
        .order_side = order_side,
        .limit_price = target_price,
        .order_quantity = rt::types::quantity{1},
      });
    fixture.cancels.push_back(
      rt::cancel_order{
        .user = fresh_user,
        .order_id = order_id,
      });
  }
}

void setup_cancel_top(const benchmark::State&)
{
  prepare_cancel_fixture(g_cancel_top_fixture, rt::types::side::buy, rt::types::price{base_bid_price});
}

void setup_cancel_deep(const benchmark::State&)
{
  const std::uint64_t deepest_bid_price = base_bid_price - (levels_per_side - 1) * price_tick;
  prepare_cancel_fixture(g_cancel_deep_fixture, rt::types::side::buy, rt::types::price{deepest_bid_price});
}

void BM_CancelAtTopOfBook(benchmark::State& state)
{
  // Cancels at the best bid. Each cancel matches top_before == best_bid, so
  // the engine emits a top_of_book whose total_at_best_bid walks the level's
  // surviving intrusive list. This exercises the cancel hot path that real
  // top-of-book churn pays.
  auto& fixture = g_cancel_top_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    fixture.engine->send(fixture.cancels[cursor++]);
  }
  state.SetItemsProcessed(state.iterations());
}

void BM_CancelDeepInBook(benchmark::State& state)
{
  // Cancels at the deepest bid level (the lowest price the seed placed).
  // top_before never matches the cancelled price, so the engine skips the
  // top_of_book emit and the cancel runs at its minimum cost: variant
  // dispatch, resting_index_ find/erase, and the book's intrusive unlink.
  auto& fixture = g_cancel_deep_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    fixture.engine->send(fixture.cancels[cursor++]);
  }
  state.SetItemsProcessed(state.iterations());
}

// Each iteration consumes one marketable order. With ~6000 seeded asks and
// ~200 orders per cross, the book supports ~25 crosses before the front
// drains; pick a comfortable margin so Setup-rebuilt repetitions never spill
// over.
constexpr std::size_t cross_iterations_per_repetition = 20;
constexpr int cross_repetitions = 30;

struct cross_fixture
{
  std::optional<bench_engine> engine;
  std::vector<rt::request> marketable_commands;
  std::uint64_t orders_consumed_per_cross = 0;
};

cross_fixture g_cross_fixture;

void setup_cross(const benchmark::State&)
{
  const auto plan = build_seed_plan();

  // Group asks by price (best first) so the cross size can stop at a level
  // boundary rather than mid-level. Per-level quantities determine the
  // marketable order's total quantity.
  std::map<rt::types::price, std::vector<seeded_order>, std::less<>> ask_levels;
  for (const auto& entry : plan) {
    if (entry.order_side != rt::types::side::sell) {
      continue;
    }
    ask_levels[entry.limit_price].push_back(entry);
  }

  std::uint64_t marketable_quantity = 0;
  std::size_t orders_consumed_actual = 0;
  for (const auto& [level_price, level_orders] : ask_levels) {
    for (const auto& level_order : level_orders) {
      marketable_quantity += level_order.order_quantity.get();
    }
    orders_consumed_actual += level_orders.size();
    if (orders_consumed_actual >= cross_target_orders) {
      break;
    }
  }

  g_cross_fixture.engine.emplace();
  seed(*g_cross_fixture.engine, plan);
  g_cross_fixture.orders_consumed_per_cross = orders_consumed_actual;

  // Marketable price sits strictly above any ask the cross might reach over
  // the whole repetition, so the marketable predicate evaluates true at every
  // touched level. After cross_iterations_per_repetition crosses the front of
  // the book has moved deeper, but stays well within the seeded range.
  const rt::types::price marketable_limit{base_ask_price + (levels_per_side + 1) * price_tick};

  g_cross_fixture.marketable_commands.clear();
  g_cross_fixture.marketable_commands.reserve(cross_iterations_per_repetition);
  for (std::size_t i = 0; i < cross_iterations_per_repetition; ++i) {
    g_cross_fixture.marketable_commands.push_back(
      rt::new_order{
        .user = fresh_user,
        .order_id = rt::types::user_order_id{fresh_id_base + i},
        .instrument = bench_symbol,
        .order_side = rt::types::side::buy,
        .limit_price = marketable_limit,
        .order_quantity = rt::types::quantity{marketable_quantity},
      });
  }
}

void BM_CrossManyOrders(benchmark::State& state)
{
  // One marketable buy per iteration, sized to consume the smallest set of
  // top ask levels whose total order count covers cross_target_orders. Each
  // iteration moves the front of the book deeper; the repetition is bounded
  // so the cross is always crossing a populated band.
  auto& fixture = g_cross_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    fixture.engine->send(fixture.marketable_commands[cursor++]);
  }

  state.SetItemsProcessed(
    static_cast<std::int64_t>(state.iterations()) * static_cast<std::int64_t>(fixture.orders_consumed_per_cross));
}

BENCHMARK(BM_PlaceLimitNoFill);
BENCHMARK(BM_PlaceCancelRoundTrip);

BENCHMARK(BM_CancelAtTopOfBook)
  ->Iterations(cancel_iterations_per_repetition)
  ->Repetitions(cancel_repetitions)
  ->Setup(setup_cancel_top);

BENCHMARK(BM_CancelDeepInBook)
  ->Iterations(cancel_iterations_per_repetition)
  ->Repetitions(cancel_repetitions)
  ->Setup(setup_cancel_deep);

BENCHMARK(BM_CrossManyOrders)->Iterations(cross_iterations_per_repetition)->Repetitions(cross_repetitions)->Setup(setup_cross);

} // namespace
