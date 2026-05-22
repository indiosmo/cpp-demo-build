#ifndef ORDER_ROUTING_TYPES_HPP
#define ORDER_ROUTING_TYPES_HPP

#include "kraken/fixed_string.hpp"
#include "kraken/strong_type.hpp"

#include <cstdint>

/*
 * Strong type vocabulary for the order_routing domain. Each primitive
 * the CSV wire protocol carries is wrapped in a distinct nominal type so
 * user_id and user_order_id (etc.) cannot be swapped at a call site.
 *
 * symbol capacity 8: longest symbol in the test corpus is "AAPL" (4
 * chars); 8 leaves headroom without inflating message payloads.
 */

namespace order_routing::types {

using user_id = kraken::strong_type<std::uint64_t, struct UserIdTag>;
using user_order_id = kraken::strong_type<std::uint64_t, struct UserOrderIdTag>;
using symbol = kraken::strong_type<kraken::fixed_string<8>, struct SymbolTag>;
using price = kraken::strong_type<std::uint64_t, struct PriceTag>;
using quantity = kraken::strong_type<std::uint64_t, struct QuantityTag>;

enum class side : std::uint8_t
{
  buy,
  sell,
};

constexpr side opposite(side value)
{
  return value == side::buy ? side::sell : side::buy;
}

} // namespace order_routing::types

#endif /* ORDER_ROUTING_TYPES_HPP */
