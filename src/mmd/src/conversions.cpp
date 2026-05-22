#include "mmd/conversions.hpp"

namespace mmd {

security_definition to_mmd(const market_data::security_definition& message)
{
  return security_definition{
    .security_id = message.security_id,
    .symbol = message.symbol,
    .security_exchange = message.security_exchange,
    .security_group = message.security_group,
    .security_type = message.security_type,
    .security_subtype = message.security_subtype,
    .min_price_increment = message.min_price_increment,
    .round_lot = message.round_lot,
    .currency = message.currency,
  };
}

security_status to_mmd(const market_data::security_status& message)
{
  return security_status{
    .security_id = message.security_id,
    .security_exchange = message.security_exchange,
    .trading_session_id = message.trading_session_id,
    .security_trading_status = message.security_trading_status,
    .security_trading_event = message.security_trading_event,
    .transact_time = message.transact_time,
  };
}

execution_summary to_mmd(const market_data::execution_summary& message)
{
  return execution_summary{
    .security_id = message.security_id,
    .aggressor_side = message.aggressor_side,
    .last_px = message.last_px,
    .fill_qty = message.fill_qty,
    .traded_hidden_qty = message.traded_hidden_qty,
    .cancel_qty = message.cancel_qty,
    .aggressor_time = message.aggressor_time,
    .transact_time = message.transact_time,
  };
}

trade to_mmd(const market_data::trade& message)
{
  return trade{
    .security_id = message.security_id,
    .trade_id = message.trade_id,
    .price = message.price,
    .quantity = message.quantity,
    .buyer = message.buyer,
    .seller = message.seller,
    .trade_condition = message.trade_condition,
    .trade_sub_type = message.trade_sub_type,
    .trade_date = message.trade_date,
    .transact_time = message.transact_time,
  };
}

mbo_book_update to_mmd(const market_data::mbo_book_update& message)
{
  return mbo_book_update{
    .security_id = message.security_id,
    .update_action = message.update_action,
    .side = message.side,
    .resting_order_id = message.resting_order_id,
    .price = message.price,
    .quantity = message.quantity,
    .previous_quantity = message.previous_quantity,
    .transact_time = message.transact_time,
  };
}

message to_mmd(const market_data::message& message)
{
  return lab::match(message, [](const auto& event) -> mmd::message { return to_mmd(event); });
}

market_data::security_definition to_market_data(const security_definition& message)
{
  return market_data::security_definition{
    .security_id = message.security_id,
    .symbol = message.symbol,
    .security_exchange = message.security_exchange,
    .security_group = message.security_group,
    .security_type = message.security_type,
    .security_subtype = message.security_subtype,
    .min_price_increment = message.min_price_increment,
    .round_lot = message.round_lot,
    .currency = message.currency,
  };
}

market_data::security_status to_market_data(const security_status& message)
{
  return market_data::security_status{
    .security_id = message.security_id,
    .security_exchange = message.security_exchange,
    .trading_session_id = message.trading_session_id,
    .security_trading_status = message.security_trading_status,
    .security_trading_event = message.security_trading_event,
    .transact_time = message.transact_time,
  };
}

market_data::execution_summary to_market_data(const execution_summary& message)
{
  return market_data::execution_summary{
    .security_id = message.security_id,
    .aggressor_side = message.aggressor_side,
    .last_px = message.last_px,
    .fill_qty = message.fill_qty,
    .traded_hidden_qty = message.traded_hidden_qty,
    .cancel_qty = message.cancel_qty,
    .aggressor_time = message.aggressor_time,
    .transact_time = message.transact_time,
  };
}

market_data::trade to_market_data(const trade& message)
{
  return market_data::trade{
    .security_id = message.security_id,
    .trade_id = message.trade_id,
    .price = message.price,
    .quantity = message.quantity,
    .buyer = message.buyer,
    .seller = message.seller,
    .trade_condition = message.trade_condition,
    .trade_sub_type = message.trade_sub_type,
    .trade_date = message.trade_date,
    .transact_time = message.transact_time,
  };
}

market_data::mbo_book_update to_market_data(const mbo_book_update& message)
{
  return market_data::mbo_book_update{
    .security_id = message.security_id,
    .update_action = message.update_action,
    .side = message.side,
    .resting_order_id = message.resting_order_id,
    .price = message.price,
    .quantity = message.quantity,
    .previous_quantity = message.previous_quantity,
    .transact_time = message.transact_time,
  };
}

market_data::message to_market_data(const message& message)
{
  return lab::match(message, [](const auto& event) -> market_data::message { return to_market_data(event); });
}

} // namespace mmd
