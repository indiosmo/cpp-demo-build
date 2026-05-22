#include "catch2/catch_test_macros.hpp"
#include "morfix/conversions.hpp"

#include <optional>
#include <variant>

TEST_CASE("morfix conversions - new order becomes FIX-shaped request", "[morfix][codec]")
{
  const mor::request request = mor::new_order_single{
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

  const auto fix_request = morfix::to_fix(request);

  REQUIRE(fix_request.has_value());
  REQUIRE(std::holds_alternative<morfix::new_order_single>(*fix_request));
  const auto& new_order = std::get<morfix::new_order_single>(*fix_request);
  CHECK(new_order.account == morfix::types::client_id{7});
  CHECK(new_order.cl_ord_id == morfix::types::cl_ord_id{42});
  CHECK(new_order.symbol == morfix::types::symbol{"PETR4"});
  CHECK(new_order.order_qty == morfix::types::quantity{100});
}

TEST_CASE("morfix conversions - lab-only flush has no FIX-shaped request", "[morfix][codec]")
{
  const mor::request request = mor::flush_request{};
  CHECK_FALSE(morfix::to_fix(request).has_value());
}

TEST_CASE("morfix conversions - FIX-shaped new order returns to normalized request", "[morfix][codec]")
{
  const morfix::request request = morfix::new_order_single{
    .account = morfix::types::client_id{7},
    .cl_ord_id = morfix::types::cl_ord_id{42},
    .security_id = morfix::types::security_id{123},
    .symbol = morfix::types::symbol{"PETR4"},
    .security_exchange = morfix::types::security_exchange{"BVMF"},
    .side = morfix::types::side::buy,
    .ord_type = morfix::types::ord_type::limit,
    .time_in_force = morfix::types::time_in_force::day,
    .order_qty = morfix::types::quantity{100},
    .price = morfix::types::price{2750},
  };

  const auto routed_request = morfix::to_mor(request);

  REQUIRE(std::holds_alternative<mor::new_order_single>(routed_request));
  const auto& order = std::get<mor::new_order_single>(routed_request);
  CHECK(order.client_id == mor::types::client_id{7});
  CHECK(order.cl_ord_id == mor::types::cl_ord_id{42});
  CHECK(order.symbol == mor::types::symbol{"PETR4"});
  CHECK(order.price == mor::types::price{2750});
}

TEST_CASE("morfix conversions - FIX-shaped execution report returns to normalized event", "[morfix][codec]")
{
  const morfix::event event = morfix::execution_report{
    .account = morfix::types::client_id{7},
    .cl_ord_id = morfix::types::cl_ord_id{42},
    .orig_cl_ord_id = std::nullopt,
    .order_id = morfix::types::order_id{1001},
    .exec_id = morfix::types::exec_id{1},
    .security_id = morfix::types::security_id{123},
    .symbol = morfix::types::symbol{"PETR4"},
    .security_exchange = morfix::types::security_exchange{"BVMF"},
    .side = morfix::types::side::buy,
    .ord_type = morfix::types::ord_type::limit,
    .time_in_force = morfix::types::time_in_force::day,
    .exec_type = morfix::types::exec_type::new_order,
    .ord_status = morfix::types::ord_status::new_order,
    .order_qty = morfix::types::quantity{100},
    .cum_qty = morfix::types::quantity{0},
    .leaves_qty = morfix::types::quantity{100},
    .last_qty = std::nullopt,
    .last_px = std::nullopt,
    .avg_px = std::nullopt,
    .transact_time = morfix::types::timestamp{123456},
    .reject_reason = std::nullopt,
    .text = "accepted",
  };

  const auto routed_event = morfix::to_mor(event);

  REQUIRE(std::holds_alternative<mor::execution_report>(routed_event));
  const auto& report = std::get<mor::execution_report>(routed_event);
  CHECK(report.client_id == mor::types::client_id{7});
  CHECK(report.order_id == mor::types::order_id{1001});
  CHECK(report.ord_status == mor::types::ord_status::new_order);
  CHECK(report.text == "accepted");
}
