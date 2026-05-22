#include "boost/pool/pool.hpp"
#include "matching_engine/order_book.hpp"
#include "matching_engine/order_state.hpp"
#include "order_routing/types.hpp"

#include <algorithm>
#include <benchmark/benchmark.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <vector>

/*
 * Order-book microbenchmarks for the production order book exposed through
 * matching_engine::order_book.
 *
 * The raw book links caller-owned order_node objects; in the application the
 * engine owns the node pool. The local fixture mirrors that ownership so the
 * benchmark can drive the real place(order_node*) / cancel(order_node*) API.
 *
 * Mutating benchmarks use the Setup-per-repetition pattern: Setup builds a
 * fresh fixture and pre-builds the iteration's worth of order_state inputs
 * (or pre-placed nodes to cancel) before the timed loop runs. The timed loop
 * walks a cursor and performs one operation per iteration, so per-iteration
 * CPU time is the per-op cost directly. Iterations(K) and Repetitions(R)
 * pin the iteration count so Setup knows exactly how many entries to build,
 * and ComputeStatistics surfaces mean/median/stddev/cv across repetitions.
 *
 * The three stateless read-only benchmarks (BM_TraverseThreePrices,
 * BM_LookupByPrice, BM_IterateFullSide) build their book once at function
 * entry and re-run the same read-only walk in the timed loop, so they do
 * not need a Setup callback.
 */

namespace {

namespace me = matching_engine;
namespace rt = order_routing;

constexpr std::size_t heavy_levels = 512;
constexpr std::size_t heavy_orders_per_level = 64;
constexpr std::size_t max_grow_depth = 1024;

constexpr std::uint64_t base_price = 100;
constexpr std::uint64_t order_quantity = 10;
constexpr rt::types::user_id benchmark_user{7};

// Fresh ids for benchmark-owned orders live well above the heavy seed range
// (max heavy id = 512 * 64 = 32,768) so they never collide with seeded ids.
constexpr std::uint64_t fresh_id_base = 1ULL << 32;

constexpr std::size_t pool_warmup_factor = 4;
constexpr std::size_t pool_chunk_size = pool_warmup_factor * max_grow_depth;

const rt::types::symbol benchmark_symbol{"BENCH"};

constexpr std::uint64_t bid_price_at(std::size_t index, std::size_t level_count)
{
  return base_price + (2 * (level_count - index));
}

constexpr std::uint64_t ask_price_at(std::size_t index)
{
  return base_price + (2 * (index + 1));
}

me::order_state make_order_state(rt::types::user_order_id order_id, rt::types::side order_side, rt::types::price limit_price)
{
  return me::order_state{
    .user = benchmark_user,
    .order_id = order_id,
    .instrument = benchmark_symbol,
    .order_side = order_side,
    .limit_price = limit_price,
    .remaining_quantity = rt::types::quantity{order_quantity},
  };
}

struct book_fixture
{
  boost::pool<> node_pool{sizeof(me::order_node), pool_chunk_size};
  me::order_book book;

  ~book_fixture()
  {
    reset();
  }

  book_fixture() = default;
  book_fixture(const book_fixture&) = delete;
  book_fixture& operator=(const book_fixture&) = delete;
  book_fixture(book_fixture&&) = delete;
  book_fixture& operator=(book_fixture&&) = delete;

  me::order_node* place(const me::order_state& resting)
  {
    void* memory = node_pool.malloc();
    if (memory == nullptr) {
      throw std::bad_alloc{};
    }

    auto* node = new (memory) me::order_node;
    node->data = resting;
    book.place(node);
    return node;
  }

  void cancel(me::order_node* node)
  {
    book.cancel(node);
    release(node);
  }

  void reset()
  {
    drain(book.bids());
    drain(book.asks());
  }

private:
  void release(me::order_node* node)
  {
    std::destroy_at(node);
    node_pool.free(node);
  }

  template <typename SideMap>
  void drain(SideMap& side_map)
  {
    while (!side_map.empty()) {
      auto& level = side_map.begin()->second;
      while (!level.orders.empty()) {
        auto& node = level.orders.front();
        level.orders.pop_front();
        release(&node);
      }
      side_map.erase(side_map.begin());
    }
  }
};

void seed_bid_book(book_fixture& fixture, std::size_t level_count, std::size_t orders_per_level)
{
  std::uint64_t next_order_id = 1;
  for (std::size_t level_index = 0; level_index < level_count; ++level_index) {
    const rt::types::price level_price{bid_price_at(level_index, level_count)};
    for (std::size_t i = 0; i < orders_per_level; ++i) {
      auto* node = fixture.place(make_order_state(rt::types::user_order_id{next_order_id++}, rt::types::side::buy, level_price));
      benchmark::DoNotOptimize(node);
    }
  }
}

void seed_ask_book(book_fixture& fixture, std::size_t level_count, std::size_t orders_per_level)
{
  std::uint64_t next_order_id = 1;
  for (std::size_t level_index = 0; level_index < level_count; ++level_index) {
    const rt::types::price level_price{ask_price_at(level_index)};
    for (std::size_t i = 0; i < orders_per_level; ++i) {
      auto* node = fixture.place(make_order_state(rt::types::user_order_id{next_order_id++}, rt::types::side::sell, level_price));
      benchmark::DoNotOptimize(node);
    }
  }
}

// Iteration / repetition counts shared by the Setup-driven benchmarks. Each
// repetition runs `iterations` operations and the framework reports mean /
// median / stddev / cv across `repetitions` samples.
constexpr std::size_t place_iterations_per_repetition = 64;
constexpr int place_repetitions = 30;
constexpr std::size_t cancel_iterations_per_repetition = 64;
constexpr int cancel_repetitions = 30;

constexpr int latency_percentile_depth = 200;
constexpr int latency_percentile_repetitions = 30;

/*
 * Place fixture used by BM_PlaceExistingPrice and BM_PlaceNewPrice. Setup
 * seeds the heavy book and pre-builds K place inputs targeting the scenario's
 * price (existing best bid, or fresh prices below the deepest seeded bid).
 * The timed loop walks the input vector with a cursor.
 */
struct place_fixture
{
  std::optional<book_fixture> book;
  std::vector<me::order_state> places;
};

place_fixture g_place_existing_fixture;
place_fixture g_place_new_fixture;

void setup_place_existing(const benchmark::State&)
{
  auto& fixture = g_place_existing_fixture;
  fixture.book.emplace();
  seed_bid_book(*fixture.book, heavy_levels, heavy_orders_per_level);

  const rt::types::price target_price{bid_price_at(0, heavy_levels)};
  fixture.places.clear();
  fixture.places.reserve(place_iterations_per_repetition);
  for (std::size_t i = 0; i < place_iterations_per_repetition; ++i) {
    fixture.places.push_back(make_order_state(rt::types::user_order_id{fresh_id_base + i}, rt::types::side::buy, target_price));
  }
}

void BM_PlaceExistingPrice(benchmark::State& state)
{
  // One place per iteration onto the existing best-bid level. The side-map
  // entry is already present, so the cost is the intrusive-list append and
  // a node-pool malloc; no side-map insert. Level depth grows by one per
  // iteration during the repetition.
  auto& fixture = g_place_existing_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    auto* node = fixture.book->place(fixture.places[cursor++]);
    benchmark::DoNotOptimize(node);
  }
  state.SetItemsProcessed(state.iterations());
}

void setup_place_new(const benchmark::State&)
{
  auto& fixture = g_place_new_fixture;
  fixture.book.emplace();
  seed_bid_book(*fixture.book, heavy_levels, heavy_orders_per_level);

  // Distinct fresh prices below the deepest seeded bid, one per iteration,
  // so every place lands on a previously unseen level. Stepping by 2 stays
  // off the seeded grid (which is also 2-step) and avoids price collisions.
  const std::uint64_t deepest_seeded = bid_price_at(heavy_levels - 1, heavy_levels);
  fixture.places.clear();
  fixture.places.reserve(place_iterations_per_repetition);
  for (std::size_t i = 0; i < place_iterations_per_repetition; ++i) {
    const rt::types::price fresh_price{deepest_seeded - 2 * (i + 1)};
    fixture.places.push_back(make_order_state(rt::types::user_order_id{fresh_id_base + i}, rt::types::side::buy, fresh_price));
  }
}

void BM_PlaceNewPrice(benchmark::State& state)
{
  // Each iteration places at a previously unseen price, forcing a side-map
  // insert in addition to the intrusive-list append. The side-map grows by
  // one entry per iteration during the repetition.
  auto& fixture = g_place_new_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    auto* node = fixture.book->place(fixture.places[cursor++]);
    benchmark::DoNotOptimize(node);
  }
  state.SetItemsProcessed(state.iterations());
}

/*
 * Growing-level fixture. Setup builds an empty book and pre-builds K place
 * inputs all targeting the same price; the timed loop walks them, so the
 * level depth grows from zero to K during the repetition. Per-iteration
 * cost is the average insert cost across that depth range.
 */
struct growing_level_fixture
{
  std::optional<book_fixture> book;
  std::vector<me::order_state> places;
};

growing_level_fixture g_growing_level_fixture;

void prepare_growing_level(std::size_t depth)
{
  auto& fixture = g_growing_level_fixture;
  fixture.book.emplace();

  const rt::types::price target_price{base_price};
  fixture.places.clear();
  fixture.places.reserve(depth);
  for (std::size_t i = 0; i < depth; ++i) {
    fixture.places.push_back(make_order_state(rt::types::user_order_id{i + 1}, rt::types::side::buy, target_price));
  }

  // Warmup pass: cycle the entire planned workload through the book once, then
  // restore the empty starting state. boost::pool allocates its first chunk
  // lazily on the first malloc, so without this pass the first timed iteration
  // pays the pool's chunk-allocation + first-touch page-fault cost (observed
  // as a 40x outlier on the first repetition). This also primes the L1 cache
  // for the fixture.places vector and the timed-loop branch predictor.
  std::vector<me::order_node*> warmup_nodes;
  warmup_nodes.reserve(depth);
  for (const auto& place : fixture.places) {
    warmup_nodes.push_back(fixture.book->place(place));
  }
  for (auto* node : warmup_nodes) {
    fixture.book->cancel(node);
  }
}

template <std::size_t Depth>
void setup_growing_level(const benchmark::State&)
{
  prepare_growing_level(Depth);
}

void BM_PlaceGrowingLevel(benchmark::State& state)
{
  auto& fixture = g_growing_level_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    auto* node = fixture.book->place(fixture.places[cursor++]);
    benchmark::DoNotOptimize(node);
  }
  state.SetItemsProcessed(state.iterations());
}

/*
 * Cancel-deep-level fixture. Setup seeds the heavy book, places K extra
 * orders at the deepest seeded level (which becomes the cancel target), and
 * records pointers to them so the timed loop can cancel one per iteration.
 * Cancel is an O(1) intrusive unlink and does not depend on level depth, so
 * the level shrinking during the run does not skew the measurement.
 */
struct cancel_node_fixture
{
  std::optional<book_fixture> book;
  std::vector<me::order_node*> targets;
};

cancel_node_fixture g_cancel_deep_fixture;

void setup_cancel_deep_level(const benchmark::State&)
{
  auto& fixture = g_cancel_deep_fixture;
  fixture.book.emplace();
  seed_bid_book(*fixture.book, heavy_levels, heavy_orders_per_level);

  const rt::types::price target_price{bid_price_at(heavy_levels - 1, heavy_levels)};
  fixture.targets.clear();
  fixture.targets.reserve(cancel_iterations_per_repetition);
  for (std::size_t i = 0; i < cancel_iterations_per_repetition; ++i) {
    auto* node =
      fixture.book->place(make_order_state(rt::types::user_order_id{fresh_id_base + i}, rt::types::side::buy, target_price));
    fixture.targets.push_back(node);
  }
}

void BM_CancelDeepLevel(benchmark::State& state)
{
  auto& fixture = g_cancel_deep_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    fixture.book->cancel(fixture.targets[cursor++]);
  }
  state.SetItemsProcessed(state.iterations());
}

/*
 * Draining-level fixture. Setup builds an empty book, fills one level with
 * K orders, and records pointers in placement order. The timed loop cancels
 * them front-to-back, draining the level over K iterations. The final
 * cancel triggers the side-map erase that empties the level entry; amortised
 * over K = 200 iterations it is a 1/200th contribution to the per-iter cost.
 */
cancel_node_fixture g_cancel_draining_fixture;

void prepare_draining_level(std::size_t depth)
{
  auto& fixture = g_cancel_draining_fixture;
  fixture.book.emplace();

  const rt::types::price target_price{base_price};
  fixture.targets.clear();
  fixture.targets.reserve(depth);
  for (std::size_t i = 0; i < depth; ++i) {
    auto* node = fixture.book->place(make_order_state(rt::types::user_order_id{i + 1}, rt::types::side::buy, target_price));
    fixture.targets.push_back(node);
  }
}

template <std::size_t Depth>
void setup_draining_level(const benchmark::State&)
{
  prepare_draining_level(Depth);
}

void BM_CancelDrainingLevel(benchmark::State& state)
{
  auto& fixture = g_cancel_draining_fixture;
  std::size_t cursor = 0;
  for (auto _ : state) {
    fixture.book->cancel(fixture.targets[cursor++]);
  }
  state.SetItemsProcessed(state.iterations());
}

/*
 * Stateless read-only benchmarks. The book is built once at function entry
 * and every iteration re-runs the same read; no Setup callback is needed.
 */
void BM_TraverseThreePrices(benchmark::State& state)
{
  book_fixture fixture;
  seed_ask_book(fixture, heavy_levels, heavy_orders_per_level);

  const std::uint64_t incoming_quantity = 3 * heavy_orders_per_level * order_quantity;

  for (auto _ : state) {
    std::uint64_t remaining = incoming_quantity;
    std::uint64_t checksum = 0;

    for (const auto& [level_price, level] : fixture.book.asks()) {
      if (remaining == 0) {
        break;
      }

      checksum += level_price;
      for (const auto& node : level.orders) {
        const std::uint64_t fill = std::min(remaining, node.data.remaining_quantity.get());
        remaining -= fill;
        checksum += node.data.order_id + fill;

        if (remaining == 0) {
          break;
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
  }

  state.SetItemsProcessed(state.iterations());
}

void BM_LookupByPrice(benchmark::State& state)
{
  const std::size_t n = static_cast<std::size_t>(state.range(0));
  book_fixture fixture;

  for (std::size_t i = 0; i < n; ++i) {
    auto* node = fixture.place(
      make_order_state(rt::types::user_order_id{i + 1}, rt::types::side::buy, rt::types::price{base_price + 2 * i}));
    benchmark::DoNotOptimize(node);
  }

  std::vector<rt::types::price> lookup_keys;
  lookup_keys.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t shuffled = (i * 2654435761ULL) % n;
    lookup_keys.push_back(rt::types::price{base_price + 2 * shuffled});
  }

  std::size_t cursor = 0;
  for (auto _ : state) {
    auto it = fixture.book.bids().find(lookup_keys[cursor]);
    benchmark::DoNotOptimize(it);
    cursor = (cursor + 1) % n;
  }

  state.SetItemsProcessed(state.iterations());
}

void BM_IterateFullSide(benchmark::State& state)
{
  const std::size_t n = static_cast<std::size_t>(state.range(0));
  book_fixture fixture;

  for (std::size_t i = 0; i < n; ++i) {
    auto* node = fixture.place(
      make_order_state(rt::types::user_order_id{i + 1}, rt::types::side::buy, rt::types::price{base_price + 2 * i}));
    benchmark::DoNotOptimize(node);
  }

  for (auto _ : state) {
    std::uint64_t checksum = 0;
    for (const auto& [price, level] : fixture.book.bids()) {
      checksum += price;
      checksum += level.orders.front().data.remaining_quantity;
    }
    benchmark::DoNotOptimize(checksum);
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}

template <int Percentile>
double percentile_aggregator(const std::vector<double>& values)
{
  if (values.empty()) {
    return 0.0;
  }
  std::vector<double> sorted(values);
  std::sort(sorted.begin(), sorted.end());
  const double rank = (static_cast<double>(Percentile) / 100.0) * static_cast<double>(sorted.size() - 1);
  const std::size_t lower = static_cast<std::size_t>(std::floor(rank));
  const std::size_t upper = std::min(lower + 1, sorted.size() - 1);
  const double fraction = rank - static_cast<double>(lower);
  return sorted[lower] + fraction * (sorted[upper] - sorted[lower]);
}

BENCHMARK(BM_PlaceExistingPrice)
  ->Name("BM_PlaceExistingPrice/current_heavy")
  ->Iterations(place_iterations_per_repetition)
  ->Repetitions(place_repetitions)
  ->Setup(setup_place_existing);

BENCHMARK(BM_PlaceNewPrice)
  ->Name("BM_PlaceNewPrice/current_heavy")
  ->Iterations(place_iterations_per_repetition)
  ->Repetitions(place_repetitions)
  ->Setup(setup_place_new);

BENCHMARK(BM_PlaceGrowingLevel)
  ->Name("BM_PlaceGrowingLevel/current_heavy/64")
  ->Iterations(64)
  ->Repetitions(place_repetitions)
  ->Setup(setup_growing_level<64>);

BENCHMARK(BM_PlaceGrowingLevel)
  ->Name("BM_PlaceGrowingLevel/current_heavy/256")
  ->Iterations(256)
  ->Repetitions(place_repetitions)
  ->Setup(setup_growing_level<256>);

BENCHMARK(BM_PlaceGrowingLevel)
  ->Name("BM_PlaceGrowingLevel/current_heavy/1024")
  ->Iterations(1024)
  ->Repetitions(place_repetitions)
  ->Setup(setup_growing_level<1024>);

BENCHMARK(BM_CancelDeepLevel)
  ->Name("BM_CancelDeepLevel/current_heavy")
  ->Iterations(cancel_iterations_per_repetition)
  ->Repetitions(cancel_repetitions)
  ->Setup(setup_cancel_deep_level);

BENCHMARK(BM_TraverseThreePrices)->Name("BM_TraverseThreePrices/current_heavy");

BENCHMARK(BM_LookupByPrice)->Name("BM_LookupByPrice/current_sweep")->Arg(10)->Arg(50)->Arg(100)->Arg(250)->Arg(500)->Arg(1000);
BENCHMARK(BM_IterateFullSide)
  ->Name("BM_IterateFullSide/current_sweep")
  ->Arg(10)
  ->Arg(50)
  ->Arg(100)
  ->Arg(250)
  ->Arg(500)
  ->Arg(1000);

BENCHMARK(BM_PlaceGrowingLevel)
  ->Name("BM_PlaceGrowingLevel/current_latency_percentiles")
  ->Iterations(latency_percentile_depth)
  ->Repetitions(latency_percentile_repetitions)
  ->Setup(setup_growing_level<latency_percentile_depth>)
  ->ComputeStatistics("p50", &percentile_aggregator<50>)
  ->ComputeStatistics("p90", &percentile_aggregator<90>)
  ->ComputeStatistics("p95", &percentile_aggregator<95>)
  ->ComputeStatistics("p99", &percentile_aggregator<99>);

BENCHMARK(BM_CancelDrainingLevel)
  ->Name("BM_CancelDrainingLevel/current_latency_percentiles")
  ->Iterations(latency_percentile_depth)
  ->Repetitions(latency_percentile_repetitions)
  ->Setup(setup_draining_level<latency_percentile_depth>)
  ->ComputeStatistics("p50", &percentile_aggregator<50>)
  ->ComputeStatistics("p90", &percentile_aggregator<90>)
  ->ComputeStatistics("p95", &percentile_aggregator<95>)
  ->ComputeStatistics("p99", &percentile_aggregator<99>);

} // namespace
