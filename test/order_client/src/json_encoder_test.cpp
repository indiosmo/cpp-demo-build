#include "catch2/catch_test_macros.hpp"
#include "lab/json.hpp"
#include "order_client/json_encoder.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

namespace {

namespace routing = order_entry;

} // namespace

TEST_CASE("json_encoder - encodes new orders", "[order_client][json_encoder]")
{
  const order_client::json_encoder encoder;

  const auto record = encoder.encode(
    routing::new_order_single{
      .client_id = routing::types::client_id{7},
      .cl_ord_id = routing::types::cl_ord_id{42},
      .security_id = routing::types::security_id{0},
      .symbol = routing::types::symbol{"IBM"},
      .security_exchange = routing::types::security_exchange{"BVMF"},
      .side = routing::types::side::buy,
      .ord_type = routing::types::ord_type::limit,
      .time_in_force = routing::types::time_in_force::day,
      .order_qty = routing::types::quantity{300},
      .price = routing::types::price{125},
    });

  const auto payload = lab::json::value::parse(record);
  CHECK(payload["message_type"] == "new_order_single");
  CHECK(payload["client_id"] == 7);
  CHECK(payload["cl_ord_id"] == 42);
  CHECK(payload["symbol"] == "IBM");
  CHECK(payload["security_exchange"] == "BVMF");
  CHECK(payload["side"] == "buy");
  CHECK(payload["ord_type"] == "limit");
  CHECK(payload["time_in_force"] == "day");
  CHECK(payload["order_qty"] == 300);
  CHECK(payload["price"] == 125);
}

TEST_CASE("json_encoder - encodes cancel orders", "[order_client][json_encoder]")
{
  const order_client::json_encoder encoder;

  const auto record = encoder.encode(
    routing::cancel_order{
      .client_id = routing::types::client_id{7},
      .cl_ord_id = routing::types::cl_ord_id{99},
      .orig_cl_ord_id = routing::types::orig_cl_ord_id{42},
    });

  const auto payload = lab::json::value::parse(record);
  CHECK(payload["message_type"] == "cancel_order");
  CHECK(payload["client_id"] == 7);
  CHECK(payload["cl_ord_id"] == 99);
  CHECK(payload["orig_cl_ord_id"] == 42);
}

TEST_CASE("json_encoder - encodes flush", "[order_client][json_encoder]")
{
  const order_client::json_encoder encoder;
  const auto payload = lab::json::value::parse(encoder.encode(routing::flush{}));

  CHECK(payload["message_type"] == "flush");
}
