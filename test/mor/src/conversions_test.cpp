#include "catch2/catch_test_macros.hpp"
#include "mor/conversions.hpp"

#include <variant>

namespace {

mor::new_order_single make_mor_order()
{
  return mor::new_order_single{
    .client_id = mor::types::client_id{7},
    .cl_ord_id = mor::types::cl_ord_id{42},
    .security_id = mor::types::security_id{123},
    .symbol = mor::types::symbol{"PETR4"},
    .security_exchange = mor::types::security_exchange{"BVMF"},
    .side = mor::types::side::buy,
    .ord_type = mor::types::ord_type::limit,
    .time_in_force = mor::types::time_in_force::day,
    .order_qty = mor::types::quantity{100},
    .price = mor::types::price{2750},
  };
}

} // namespace

TEST_CASE("mor conversions - order_entry new order roundtrip", "[mor][codec]")
{
  const auto request = make_mor_order();
  const auto order_entry_request = mor::to_order_entry(request);
  const auto routed_request = mor::to_mor(order_entry_request);

  CHECK(routed_request.client_id == request.client_id);
  CHECK(routed_request.cl_ord_id == request.cl_ord_id);
  CHECK(routed_request.security_id == request.security_id);
  CHECK(routed_request.symbol == request.symbol);
  CHECK(routed_request.security_exchange == request.security_exchange);
  CHECK(routed_request.side == request.side);
  CHECK(routed_request.ord_type == request.ord_type);
  CHECK(routed_request.time_in_force == request.time_in_force);
  CHECK(routed_request.order_qty == request.order_qty);
  CHECK(routed_request.price == request.price);
}

TEST_CASE("mor conversions - order_entry request variant keeps replace identity", "[mor][codec]")
{
  const order_entry::request request = order_entry::replace_order{
    .client_id = order_entry::types::client_id{7},
    .cl_ord_id = order_entry::types::cl_ord_id{43},
    .orig_cl_ord_id = order_entry::types::orig_cl_ord_id{42},
    .security_id = order_entry::types::security_id{123},
    .symbol = order_entry::types::symbol{"PETR4"},
    .security_exchange = order_entry::types::security_exchange{"BVMF"},
    .side = order_entry::types::side::sell,
    .ord_type = order_entry::types::ord_type::limit,
    .time_in_force = order_entry::types::time_in_force::day,
    .order_qty = order_entry::types::quantity{80},
    .price = order_entry::types::price{2760},
  };

  const auto routed_request = mor::to_mor(request);
  REQUIRE(std::holds_alternative<mor::replace_request>(routed_request));

  const auto& replace = std::get<mor::replace_request>(routed_request);
  CHECK(replace.cl_ord_id == mor::types::cl_ord_id{43});
  CHECK(replace.orig_cl_ord_id == mor::types::orig_cl_ord_id{42});
  CHECK(replace.price == mor::types::price{2760});
}

