#ifndef MATCHING_ENGINE_CONVERSIONS_HPP
#define MATCHING_ENGINE_CONVERSIONS_HPP

#include "market_data/types.hpp"
#include "order_routing/types.hpp"

#include <exception>

/*
 * Cross-domain conversions used by the matching_engine composition layer:
 * order_routing wire-side vocabulary in, market_data emit-side vocabulary
 * out. Free functions in the matching_engine namespace; engines include
 * this header rather than re-deriving the mapping at each emit site.
 */

namespace matching_engine {

constexpr market_data::types::side to_market_side(order_routing::types::side value)
{
  switch (value) {
    case order_routing::types::side::buy:
      return market_data::types::side::buy;
    case order_routing::types::side::sell:
      return market_data::types::side::sell;
  }
  std::terminate();
}

} // namespace matching_engine

#endif /* MATCHING_ENGINE_CONVERSIONS_HPP */
