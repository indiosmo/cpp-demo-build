#ifndef MATCHING_ENGINE_TYPES_HPP
#define MATCHING_ENGINE_TYPES_HPP

#include "order_routing/types.hpp"

#include "kraken/hash.hpp"

/*
 * Strong type vocabulary for the matching_engine domain. Engine-local
 * types that compose order_routing primitives live here; the engine's
 * own working-order payload type (`order_state`) stays in order_state.hpp.
 */

namespace matching_engine::types {

/*
 * Composite identity key for the cross-symbol resting-order index.
 * Namespace-scope aggregate so kraken::auto_hash can reflect over its
 * fields via boost::pfr.
 */
struct order_key
{
  order_routing::types::user_id user;
  order_routing::types::user_order_id order_id;

  friend bool operator==(const order_key&, const order_key&) = default;
};

// boost::unordered_flat_map (the resting-order index) consults boost::hash
// via ADL, not std::hash; declare hash_value at namespace scope.
KRAKEN_HASH_VALUE(order_key)

} // namespace matching_engine::types

KRAKEN_STD_HASH(matching_engine::types::order_key)

#endif /* MATCHING_ENGINE_TYPES_HPP */
