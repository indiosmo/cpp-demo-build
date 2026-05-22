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

TEST_CASE("ospec b3 - normalizes market-data values", "[ospec][codec]")
{
  CHECK(ospec::b3::normalize(mmd::types::side::buy) == '0');
  CHECK(ospec::b3::normalize(mmd::types::side::sell) == '1');
  CHECK(ospec::b3::normalize(mmd::types::update_action::new_order) == '0');
  CHECK(ospec::b3::normalize(mmd::types::update_action::change) == '1');
  CHECK(ospec::b3::normalize(mmd::types::update_action::delete_order) == '2');
  CHECK(ospec::b3::normalize(mmd::types::trade_condition::regular) == '0');
}

