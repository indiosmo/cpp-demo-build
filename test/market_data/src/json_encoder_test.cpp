#include "catch2/catch_test_macros.hpp"
#include "lab/json.hpp"
#include "market_data/json_encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/types.hpp"

#include <optional>
#include <string_view>

namespace {

namespace md = market_data;
namespace detail = md::json_encoder_detail;

template <typename Message>
lab::json::value encode_message(const Message& msg)
{
  const md::json_encoder encoder;
  return lab::json::value::parse(encoder.encode(md::message{msg}));
}

} // namespace

TEST_CASE("json_encoder - encode execution_summary", "[market_data][json_encoder][format]")
{
  const auto payload = encode_message(
    md::execution_summary{
      .security_id = md::types::security_id{1},
      .aggressor_side = md::types::side::sell,
      .last_px = md::types::price{10},
      .fill_qty = md::types::quantity{100},
      .traded_hidden_qty = std::nullopt,
      .cancel_qty = std::nullopt,
      .aggressor_time = md::types::timestamp{0},
      .transact_time = md::types::timestamp{0},
    });

  CHECK(payload["message_type"] == "execution_summary");
  CHECK(payload["security_id"] == 1);
  CHECK(payload["aggressor_side"] == "sell");
  CHECK(payload["last_px"] == 10);
  CHECK(payload["fill_qty"] == 100);
  CHECK(payload["traded_hidden_qty"].is_null());
  CHECK(payload["cancel_qty"].is_null());
  CHECK(payload["aggressor_time"] == 0);
  CHECK(payload["transact_time"] == 0);
}

TEST_CASE("json_encoder - encode trade", "[market_data][json_encoder][format]")
{
  const auto payload = encode_message(
    md::trade{
      .security_id = md::types::security_id{1},
      .trade_id = md::types::trade_id{7},
      .price = md::types::price{11},
      .quantity = md::types::quantity{20},
      .buyer = md::types::order_id{3},
      .seller = md::types::order_id{102},
      .trade_condition = md::types::trade_condition::regular,
      .trade_sub_type = std::nullopt,
      .trade_date = md::types::trade_date{"20260522"},
      .transact_time = md::types::timestamp{0},
    });

  CHECK(payload["message_type"] == "trade");
  CHECK(payload["security_id"] == 1);
  CHECK(payload["trade_id"] == 7);
  CHECK(payload["price"] == 11);
  CHECK(payload["quantity"] == 20);
  CHECK(payload["buyer"] == 3);
  CHECK(payload["seller"] == 102);
  CHECK(payload["trade_condition"] == "regular");
  CHECK(payload["trade_sub_type"].is_null());
  CHECK(payload["trade_date"] == "20260522");
  CHECK(payload["transact_time"] == 0);
}

TEST_CASE("json_encoder - encode mbo_book_update", "[market_data][json_encoder][format]")
{
  const auto payload = encode_message(
    md::mbo_book_update{
      .security_id = md::types::security_id{1},
      .update_action = md::types::update_action::change,
      .side = md::types::side::buy,
      .resting_order_id = md::types::order_id{0},
      .price = md::types::price{10},
      .quantity = md::types::quantity{150},
      .previous_quantity = std::nullopt,
      .transact_time = md::types::timestamp{0},
    });

  CHECK(payload["message_type"] == "mbo_book_update");
  CHECK(payload["security_id"] == 1);
  CHECK(payload["update_action"] == "change");
  CHECK(payload["side"] == "buy");
  CHECK(payload["resting_order_id"] == 0);
  CHECK(payload["price"] == 10);
  CHECK(payload["quantity"] == 150);
  CHECK(payload["previous_quantity"].is_null());
  CHECK(payload["transact_time"] == 0);
}

TEST_CASE("json_encoder_detail - encode enum tokens", "[market_data][json_encoder][format]")
{
  CHECK(detail::encode_side(md::types::side::buy) == std::string_view{"buy"});
  CHECK(detail::encode_side(md::types::side::sell) == std::string_view{"sell"});
  CHECK(detail::encode_update_action(md::types::update_action::new_order) == std::string_view{"new_order"});
  CHECK(detail::encode_update_action(md::types::update_action::change) == std::string_view{"change"});
  CHECK(detail::encode_update_action(md::types::update_action::delete_order) == std::string_view{"delete_order"});
}
