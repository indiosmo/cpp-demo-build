#ifndef ORDER_ENTRY_TYPES_HPP
#define ORDER_ENTRY_TYPES_HPP

#include "lab/fixed_string.hpp"
#include "lab/strong_type.hpp"

#include <cstdint>

/*
 * Strong type vocabulary for the order-entry domain. These primitives model
 * the client-facing lifecycle contract, so they remain distinct from the
 * matching and market-data primitives that may carry the same raw values.
 */

namespace order_entry::types {

using client_id = lab::strong_type<std::uint64_t, struct ClientIdTag>;
using cl_ord_id = lab::strong_type<std::uint64_t, struct ClOrdIdTag>;
using orig_cl_ord_id = lab::strong_type<std::uint64_t, struct OrigClOrdIdTag>;
using order_id = lab::strong_type<std::uint64_t, struct OrderIdTag>;
using exec_id = lab::strong_type<std::uint64_t, struct ExecIdTag>;
using security_id = lab::strong_type<std::uint64_t, struct SecurityIdTag>;
using symbol = lab::strong_type<lab::fixed_string<16>, struct SymbolTag>;
using security_exchange = lab::strong_type<lab::fixed_string<8>, struct SecurityExchangeTag>;
using price = lab::strong_type<std::uint64_t, struct PriceTag>;
using quantity = lab::strong_type<std::uint64_t, struct QuantityTag>;
using timestamp = lab::strong_type<std::uint64_t, struct TimestampTag>;

enum class side : std::uint8_t
{
  buy,
  sell,
};

enum class ord_type : std::uint8_t
{
  market,
  limit,
};

enum class time_in_force : std::uint8_t
{
  day,
  ioc,
};

enum class exec_type : std::uint8_t
{
  new_order,
  replaced,
  canceled,
  trade,
  rejected,
  expired,
};

enum class ord_status : std::uint8_t
{
  new_order,
  partially_filled,
  filled,
  canceled,
  replaced,
  rejected,
  expired,
};

enum class reject_reason : std::uint8_t
{
  unknown_order,
  duplicate_order,
  unknown_symbol,
  unsupported_request,
};

constexpr side opposite(side value)
{
  return value == side::buy ? side::sell : side::buy;
}

} // namespace order_entry::types

#endif /* ORDER_ENTRY_TYPES_HPP */
