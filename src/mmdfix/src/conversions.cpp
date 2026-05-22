#include "mmdfix/conversions.hpp"

namespace mmdfix {

market_data_incremental_refresh to_fix(const mmd::mbo_book_update& event)
{
  return market_data_incremental_refresh{
    .security_id = event.security_id,
    .side = event.side,
    .update_action = event.update_action,
    .price = event.price,
    .quantity = event.quantity,
    .transact_time = event.transact_time,
  };
}

trade_capture_report to_fix(const mmd::trade& event)
{
  return trade_capture_report{
    .security_id = event.security_id,
    .trade_id = event.trade_id,
    .price = event.price,
    .quantity = event.quantity,
    .trade_date = event.trade_date,
    .transact_time = event.transact_time,
  };
}

} // namespace mmdfix
