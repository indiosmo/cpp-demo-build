#ifndef MARKET_DATA_TYPES_HPP
#define MARKET_DATA_TYPES_HPP

#include "lab/strong_type.hpp"

#include <cstdint>

/*
 * Strong type vocabulary for the market_data domain. Each primitive the
 * outbound CSV protocol carries is wrapped in a distinct nominal type so the
 * formatter cannot transpose, for example, a price with a total_quantity.
 */

namespace market_data::types {

using user_id = lab::strong_type<std::uint64_t, struct UserIdTag>;
using user_order_id = lab::strong_type<std::uint64_t, struct UserOrderIdTag>;
using price = lab::strong_type<std::uint64_t, struct PriceTag>;
using quantity = lab::strong_type<std::uint64_t, struct QuantityTag>;
using total_quantity = lab::strong_type<std::uint64_t, struct TotalQuantityTag>;

enum class side : std::uint8_t
{
  buy,
  sell,
};

} // namespace market_data::types

#endif /* MARKET_DATA_TYPES_HPP */
