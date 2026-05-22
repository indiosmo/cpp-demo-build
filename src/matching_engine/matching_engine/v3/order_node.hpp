#ifndef MATCHING_ENGINE_V3_ORDER_NODE_HPP
#define MATCHING_ENGINE_V3_ORDER_NODE_HPP

#include "boost/intrusive/link_mode.hpp"
#include "boost/intrusive/list_hook.hpp"
#include "matching_engine/order_state.hpp"

#include <type_traits>

/*
 * Intrusive-list hook wrapping `order_state`. Slots come from the engine's
 * pool; the v3 order_book only links them.
 *
 * Shutdown leaves nodes linked and lets the pool reclaim its memory in
 * one shot, without running per-node destructors. The static_assert
 * below pins the triviality of `order_state` that makes this safe.
 *
 * The hook is configured to skip the on-destruction "still linked"
 * check, which would otherwise fire during that bulk shutdown.
 */

namespace matching_engine::v3 {

struct order_node : boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link>>
{
  order_state data;
};

// Check the payload only: the intrusive hook drags in a client_id-defined
// (empty) destructor, so order_node itself is never trivially
// destructible even when its data is.
static_assert(std::is_trivially_destructible_v<order_state>);

} // namespace matching_engine::v3

#endif /* MATCHING_ENGINE_V3_ORDER_NODE_HPP */
