#include "catch2/catch_test_macros.hpp"
#include "morfix/conversions.hpp"

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
