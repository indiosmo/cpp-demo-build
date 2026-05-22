#include "catch2/catch_test_macros.hpp"
#include "order_client/csv_encoder.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

#include <string>

namespace {

namespace routing = order_entry;

} // namespace

TEST_CASE("csv_encoder - encodes new orders", "[order_client][csv_encoder]")
{
  const order_client::csv_encoder encoder;

  const auto record = encoder.encode(routing::new_order_single{
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

  CHECK(record == "N,7,IBM,125,300,B,42");
}

TEST_CASE("csv_encoder - encodes cancel orders", "[order_client][csv_encoder]")
{
  const order_client::csv_encoder encoder;

  const auto record = encoder.encode(routing::cancel_order{
    .client_id = routing::types::client_id{7},
    .cl_ord_id = routing::types::cl_ord_id{99},
    .orig_cl_ord_id = routing::types::orig_cl_ord_id{42},
  });

  CHECK(record == "C,7,42");
}

TEST_CASE("csv_encoder - encodes flush", "[order_client][csv_encoder]")
{
  const order_client::csv_encoder encoder;

  CHECK(encoder.encode(routing::flush{}) == "F");
}
