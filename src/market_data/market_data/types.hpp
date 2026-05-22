#ifndef MARKET_DATA_TYPES_HPP
#define MARKET_DATA_TYPES_HPP

#include "lab/fixed_string.hpp"
#include "lab/strong_type.hpp"

#include <cstdint>

/*
 * Strong type vocabulary for market-data events. These types intentionally
 * mirror raw values also present in order entry, but remain nominally
 * separate because the two domains have different lifecycle contracts.
 */

namespace market_data::types {

using security_id = lab::strong_type<std::uint64_t, struct SecurityIdTag>;
using symbol = lab::strong_type<lab::fixed_string<16>, struct SymbolTag>;
using security_exchange = lab::strong_type<lab::fixed_string<8>, struct SecurityExchangeTag>;
using security_group = lab::strong_type<lab::fixed_string<16>, struct SecurityGroupTag>;
using security_type = lab::strong_type<lab::fixed_string<8>, struct SecurityTypeTag>;
using security_subtype = lab::strong_type<lab::fixed_string<8>, struct SecuritySubtypeTag>;
using price = lab::strong_type<std::uint64_t, struct PriceTag>;
using quantity = lab::strong_type<std::uint64_t, struct QuantityTag>;
using order_id = lab::strong_type<std::uint64_t, struct OrderIdTag>;
using trade_id = lab::strong_type<std::uint64_t, struct TradeIdTag>;
using timestamp = lab::strong_type<std::uint64_t, struct TimestampTag>;
using trade_date = lab::strong_type<lab::fixed_string<8>, struct TradeDateTag>;
using currency = lab::strong_type<lab::fixed_string<3>, struct CurrencyTag>;
using trading_session_id = lab::strong_type<lab::fixed_string<8>, struct TradingSessionIdTag>;

enum class side : std::uint8_t
{
  buy,
  sell,
};

enum class update_action : std::uint8_t
{
  new_order,
  change,
  delete_order,
};

enum class security_trading_status : std::uint8_t
{
  open,
  closed,
  halted,
};

enum class security_trading_event : std::uint8_t
{
  none,
  trading_resume,
  trading_halt,
};

enum class trade_condition : std::uint8_t
{
  regular,
};

enum class trade_sub_type : std::uint8_t
{
  regular,
};

} // namespace market_data::types

#endif /* MARKET_DATA_TYPES_HPP */
