#ifndef MATCHING_ENGINE_RUNTIME_ENGINE_CONFIG_HPP
#define MATCHING_ENGINE_RUNTIME_ENGINE_CONFIG_HPP

#include "lab/defaulted_field.hpp"
#include "lab/json.hpp"
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
  LAB_DEFAULTED_FIELD(std::size_t, expected_resting_orders, default_expected_resting_orders);
  LAB_DEFAULTED_FIELD(std::size_t, node_pool_chunk_size, default_node_pool_chunk_size);
};

LAB_AUTO_JSON(engine_config, valid_symbols, expected_resting_orders, node_pool_chunk_size)

} // namespace matching_engine::runtime

#endif /* MATCHING_ENGINE_RUNTIME_ENGINE_CONFIG_HPP */
