#include "catch2/catch_test_macros.hpp"
#include "ospec/b3.hpp"

TEST_CASE("ospec b3 - exposes reference spec anchors", "[ospec][unit]")
{
  CHECK(ospec::b3::entrypoint_reference == "b3-entrypoint-messages-8.4.2.xml");
  CHECK(ospec::b3::market_data_reference == "b3-market-data-messages-2.2.0.xml");
  CHECK(ospec::b3::cl_ord_id.number == 11);
  CHECK(ospec::b3::security_id.number == 48);
}

TEST_CASE("ospec b3 - normalizes order-routing values", "[ospec][codec]")
{
  CHECK(ospec::b3::normalize(mor::types::side::buy) == '1');
  CHECK(ospec::b3::normalize(mor::types::side::sell) == '2');
  CHECK(ospec::b3::normalize(mor::types::ord_type::market) == '1');
  CHECK(ospec::b3::normalize(mor::types::ord_type::limit) == '2');
  CHECK(ospec::b3::normalize(mor::types::time_in_force::day) == '0');
  CHECK(ospec::b3::normalize(mor::types::time_in_force::ioc) == '3');
  CHECK(ospec::b3::normalize(mor::types::exec_type::trade) == 'F');
  CHECK(ospec::b3::normalize(mor::types::ord_status::filled) == '2');
}

TEST_CASE("ospec b3 - parses order-routing values", "[ospec][codec]")
{
  auto side = ospec::b3::parse_side('1');
  REQUIRE(side);
  CHECK(side.value() == mor::types::side::buy);

  auto ord_type = ospec::b3::parse_ord_type('2');
  REQUIRE(ord_type);
  CHECK(ord_type.value() == mor::types::ord_type::limit);

  auto time_in_force = ospec::b3::parse_time_in_force('3');
  REQUIRE(time_in_force);
  CHECK(time_in_force.value() == mor::types::time_in_force::ioc);

  auto exec_type = ospec::b3::parse_exec_type('F');
  REQUIRE(exec_type);
  CHECK(exec_type.value() == mor::types::exec_type::trade);

  auto ord_status = ospec::b3::parse_ord_status('4');
  REQUIRE(ord_status);
  CHECK(ord_status.value() == mor::types::ord_status::canceled);

  auto reject_reason = ospec::b3::parse_reject_reason(6);
  REQUIRE(reject_reason);
  CHECK(reject_reason.value() == mor::types::reject_reason::duplicate_order);
}

TEST_CASE("ospec b3 - normalizes market-data values", "[ospec][codec]")
{
  CHECK(ospec::b3::normalize(mmd::types::side::buy) == '0');
  CHECK(ospec::b3::normalize(mmd::types::side::sell) == '1');
  CHECK(ospec::b3::normalize(mmd::types::update_action::new_order) == '0');
  CHECK(ospec::b3::normalize(mmd::types::update_action::change) == '1');
  CHECK(ospec::b3::normalize(mmd::types::update_action::delete_order) == '2');
  CHECK(ospec::b3::normalize(mmd::types::trade_condition::regular) == '0');
}
