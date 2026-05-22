#include "catch2/catch_test_macros.hpp"
#include "order_client/csv_encoder.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include <string>

namespace {

namespace routing = order_routing;

} // namespace

TEST_CASE("csv_encoder - encodes new orders", "[order_client][csv_encoder]")
{
  const order_client::csv_encoder encoder;

  const auto record = encoder.encode(routing::new_order{
    .user = routing::types::user_id{7},
    .order_id = routing::types::user_order_id{42},
    .instrument = routing::types::symbol{"IBM"},
    .order_side = routing::types::side::buy,
    .limit_price = routing::types::price{125},
    .order_quantity = routing::types::quantity{300},
  });

  CHECK(record == "N,7,IBM,125,300,B,42");
}

TEST_CASE("csv_encoder - encodes cancel orders", "[order_client][csv_encoder]")
{
  const order_client::csv_encoder encoder;

  const auto record = encoder.encode(routing::cancel_order{
    .user = routing::types::user_id{7},
    .order_id = routing::types::user_order_id{42},
  });

  CHECK(record == "C,7,42");
}

TEST_CASE("csv_encoder - encodes flush", "[order_client][csv_encoder]")
{
  const order_client::csv_encoder encoder;

  CHECK(encoder.encode(routing::flush{}) == "F");
}
