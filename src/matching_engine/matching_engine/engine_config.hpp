#ifndef MATCHING_ENGINE_ENGINE_CONFIG_HPP
#define MATCHING_ENGINE_ENGINE_CONFIG_HPP

#include "order_routing/types.hpp"

#include <cstddef>
#include <vector>

/*
 * Strictly-explicit startup config for matching_engine::engine.
 * Deployment defaults live on matching_engine::runtime::engine_config.
 *
 *   valid_symbols            Tradable instruments; one book preallocated per entry.
 *   expected_resting_orders  Reserve hint for the identity index and node pool.
 *   node_pool_chunk_size     Chunk size for the engine-wide resting-order pool.
 */

namespace matching_engine {

struct engine_config
{
  std::vector<order_routing::types::symbol> valid_symbols;
  std::size_t expected_resting_orders;
  std::size_t node_pool_chunk_size;
};

} // namespace matching_engine

#endif /* MATCHING_ENGINE_ENGINE_CONFIG_HPP */
