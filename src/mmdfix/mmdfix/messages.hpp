#ifndef MMDFIX_MESSAGES_HPP
#define MMDFIX_MESSAGES_HPP

#include "mmd/messages.hpp"

#include <optional>
#include <variant>

namespace mmdfix {

namespace types = mmd::types;

struct market_data_incremental_refresh
{
  types::security_id security_id;
  types::side side;
  types::update_action update_action;
  std::optional<types::price> price;
  std::optional<types::quantity> quantity;
  types::timestamp transact_time;
};

struct trade_capture_report
{
  types::security_id security_id;
  types::trade_id trade_id;
  types::price price;
  types::quantity quantity;
  types::trade_date trade_date;
  types::timestamp transact_time;
};

using message = std::variant<market_data_incremental_refresh, trade_capture_report>;

} // namespace mmdfix

#endif /* MMDFIX_MESSAGES_HPP */
