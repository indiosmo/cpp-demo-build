#include "catch2/catch_test_macros.hpp"
#include "lab/json.hpp"
#include "mmd/conversions.hpp"
#include "mmd_json/json_encoder.hpp"

#include <optional>

TEST_CASE("mmd_json - encodes current trade shape through normalized boundary", "[mmd_json][codec]")
{
  const auto event = mmd::to_mmd(
    market_data::trade{
      .security_id = market_data::types::security_id{1},
      .trade_id = market_data::types::trade_id{7},
      .price = market_data::types::price{11},
      .quantity = market_data::types::quantity{20},
      .buyer = market_data::types::order_id{3},
      .seller = market_data::types::order_id{102},
      .trade_condition = market_data::types::trade_condition::regular,
      .trade_sub_type = std::nullopt,
      .trade_date = market_data::types::trade_date{"20260522"},
      .transact_time = market_data::types::timestamp{0},
    });

  const mmd_json::json_encoder encoder;
  const auto payload = lab::json::value::parse(encoder.encode(mmd::message{event}));

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

TEST_CASE("mmd_json - encodes current book update shape through normalized boundary", "[mmd_json][codec]")
{
  const auto event = mmd::to_mmd(
    market_data::mbo_book_update{
      .security_id = market_data::types::security_id{1},
      .update_action = market_data::types::update_action::change,
      .side = market_data::types::side::buy,
      .resting_order_id = market_data::types::order_id{0},
      .price = market_data::types::price{10},
      .quantity = market_data::types::quantity{150},
      .previous_quantity = std::nullopt,
      .transact_time = market_data::types::timestamp{0},
    });

  const mmd_json::json_encoder encoder;
  const auto payload = lab::json::value::parse(encoder.encode(mmd::message{event}));

  CHECK(payload["message_type"] == "mbo_book_update");
  CHECK(payload["security_id"] == 1);
  CHECK(payload["update_action"] == "change");
  CHECK(payload["side"] == "buy");
  CHECK(payload["price"] == 10);
  CHECK(payload["quantity"] == 150);
  CHECK(payload["previous_quantity"].is_null());
  CHECK(payload["transact_time"] == 0);
}

