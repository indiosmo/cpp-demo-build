#ifndef MATCHING_ENGINE_RUNTIME_ENGINE_CONFIG_HPP
#define MATCHING_ENGINE_RUNTIME_ENGINE_CONFIG_HPP

#include "order_entry/types.hpp"

#include <cstddef>
#include <vector>

/*
 * Configuration surface for matching_engine::runtime::engine. Mirrors
 * the domain engine_config plus deployment defaults. valid_symbols is
 * the only required field.
 */

namespace matching_engine::runtime {

struct engine_config
{
  // Modest starting points; deployments with deeper books or larger
  // peaks should raise both. expected_resting_orders warms the node pool
  // and reserves the identity index at startup.
  static constexpr std::size_t default_node_pool_chunk_size = 32;
  static constexpr std::size_t default_expected_resting_orders = 1024;

  std::vector<order_entry::types::symbol> valid_symbols;
  std::size_t expected_resting_orders = default_expected_resting_orders;
  std::size_t node_pool_chunk_size = default_node_pool_chunk_size;
};

} // namespace matching_engine::runtime

#endif /* MATCHING_ENGINE_RUNTIME_ENGINE_CONFIG_HPP */
