#ifndef OSPEC_B3_HPP
#define OSPEC_B3_HPP

#include "lab/result.hpp"
#include "mmd/messages.hpp"
#include "mor/messages.hpp"

#include <cstdint>
#include <string_view>

namespace ospec::b3 {

using tag = std::uint16_t;

struct field_spec
{
  tag number;
  std::string_view name;
};

inline constexpr std::string_view entrypoint_reference = "b3-entrypoint-messages-8.4.2.xml";
inline constexpr std::string_view market_data_reference = "b3-market-data-messages-2.2.0.xml";

inline constexpr field_spec account{1, "Account"};
inline constexpr field_spec avg_px{6, "AvgPx"};
inline constexpr field_spec cum_qty{14, "CumQty"};
inline constexpr field_spec cl_ord_id{11, "ClOrdID"};
inline constexpr field_spec last_px{31, "LastPx"};
inline constexpr field_spec last_qty{32, "LastQty"};
inline constexpr field_spec order_id{37, "OrderID"};
inline constexpr field_spec orig_cl_ord_id{41, "OrigClOrdID"};
inline constexpr field_spec order_qty{38, "OrderQty"};
inline constexpr field_spec ord_status{39, "OrdStatus"};
inline constexpr field_spec ord_type{40, "OrdType"};
inline constexpr field_spec price{44, "Price"};
inline constexpr field_spec security_id{48, "SecurityID"};
inline constexpr field_spec text{58, "Text"};
inline constexpr field_spec side{54, "Side"};
inline constexpr field_spec symbol{55, "Symbol"};
inline constexpr field_spec time_in_force{59, "TimeInForce"};
inline constexpr field_spec transact_time{60, "TransactTime"};
inline constexpr field_spec cxl_rej_reason{102, "CxlRejReason"};
inline constexpr field_spec leaves_qty{151, "LeavesQty"};
inline constexpr field_spec security_exchange{207, "SecurityExchange"};
inline constexpr field_spec exec_id{17, "ExecID"};
inline constexpr field_spec exec_type{150, "ExecType"};
inline constexpr field_spec md_update_action{279, "MDUpdateAction"};
inline constexpr field_spec md_entry_type{269, "MDEntryType"};
inline constexpr field_spec trade_condition{277, "TradeCondition"};

[[nodiscard]] char normalize(mor::types::side value);
[[nodiscard]] char normalize(mor::types::ord_type value);
[[nodiscard]] char normalize(mor::types::time_in_force value);
[[nodiscard]] char normalize(mor::types::exec_type value);
[[nodiscard]] char normalize(mor::types::ord_status value);
[[nodiscard]] std::uint16_t normalize(mor::types::reject_reason value);

[[nodiscard]] lab::result<mor::types::side> parse_side(char value);
[[nodiscard]] lab::result<mor::types::ord_type> parse_ord_type(char value);
[[nodiscard]] lab::result<mor::types::time_in_force> parse_time_in_force(char value);
[[nodiscard]] lab::result<mor::types::exec_type> parse_exec_type(char value);
[[nodiscard]] lab::result<mor::types::ord_status> parse_ord_status(char value);
[[nodiscard]] lab::result<mor::types::reject_reason> parse_reject_reason(std::uint16_t value);

[[nodiscard]] char normalize(mmd::types::side value);
[[nodiscard]] char normalize(mmd::types::update_action value);
[[nodiscard]] char normalize(mmd::types::trade_condition value);

} // namespace ospec::b3

#endif /* OSPEC_B3_HPP */
