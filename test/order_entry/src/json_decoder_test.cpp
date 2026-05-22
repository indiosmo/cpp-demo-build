#include "catch2/catch_test_macros.hpp"
#include "order_entry/json_decoder.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

#include <variant>

namespace {

namespace routing = order_entry;
namespace detail = routing::json_decoder_detail;

routing::json_decoder make_decoder()
{
  return routing::json_decoder{
    routing::json_decoder_config{
      .max_datagram_size = 65535,
    }};
}

} // namespace

TEST_CASE("json_decoder_detail - decode_new_order", "[order_entry][json_decoder][parse]")
{
  const auto command = detail::decode_new_order({
    {"message_type", "new_order_single"},
    {"client_id", 1},
    {"cl_ord_id", 10},
    {"symbol", "IBM"},
    {"side", "buy"},
    {"order_qty", 100},
    {"price", 25},
  });

  CHECK(command.client_id == routing::types::client_id{1});
  CHECK(command.cl_ord_id == routing::types::cl_ord_id{10});
  CHECK(command.security_id == routing::types::security_id{0});
  CHECK(command.symbol == routing::types::symbol{"IBM"});
  CHECK(command.security_exchange == routing::types::security_exchange{"BVMF"});
  CHECK(command.side == routing::types::side::buy);
  CHECK(command.ord_type == routing::types::ord_type::limit);
  CHECK(command.time_in_force == routing::types::time_in_force::day);
  CHECK(command.order_qty == routing::types::quantity{100});
  CHECK(command.price == routing::types::price{25});
}

TEST_CASE("json_decoder_detail - decode_cancel_order", "[order_entry][json_decoder][parse]")
{
  const auto command = detail::decode_cancel_order({
    {"message_type", "cancel_order"},
    {"client_id", 7},
    {"cl_ord_id", 99},
    {"orig_cl_ord_id", 42},
  });

  CHECK(command.client_id == routing::types::client_id{7});
  CHECK(command.cl_ord_id == routing::types::cl_ord_id{99});
  CHECK(command.orig_cl_ord_id == routing::types::orig_cl_ord_id{42});
}

TEST_CASE("json_decoder_detail - decode enum tokens", "[order_entry][json_decoder][parse]")
{
  CHECK(detail::decode_side("buy") == routing::types::side::buy);
  CHECK(detail::decode_side("sell") == routing::types::side::sell);
  CHECK(detail::decode_ord_type("market") == routing::types::ord_type::market);
  CHECK(detail::decode_ord_type("limit") == routing::types::ord_type::limit);
  CHECK(detail::decode_time_in_force("day") == routing::types::time_in_force::day);
  CHECK(detail::decode_time_in_force("ioc") == routing::types::time_in_force::ioc);
}

TEST_CASE("json_decoder - decode dispatches by message_type", "[order_entry][json_decoder][dispatch]")
{
  auto decoder = make_decoder();

  const auto new_order = decoder.decode(
    R"({"message_type":"new_order_single","client_id":1,"cl_ord_id":10,"symbol":"IBM","side":"buy","order_qty":100,"price":25})");
  REQUIRE(new_order);
  CHECK(std::holds_alternative<routing::new_order_single>(*new_order));

  const auto replace = decoder.decode(
    R"({"message_type":"replace_order","client_id":1,"cl_ord_id":11,"orig_cl_ord_id":10,"symbol":"IBM","side":"buy","order_qty":90,"price":26})");
  REQUIRE(replace);
  CHECK(std::holds_alternative<routing::replace_order>(*replace));

  const auto cancel = decoder.decode(R"({"message_type":"cancel_order","client_id":7,"cl_ord_id":99,"orig_cl_ord_id":42})");
  REQUIRE(cancel);
  CHECK(std::holds_alternative<routing::cancel_order>(*cancel));

  const auto flush = decoder.decode(R"({"message_type":"flush"})");
  REQUIRE(flush);
  CHECK(std::holds_alternative<routing::flush>(*flush));
}

TEST_CASE("json_decoder - reports invalid json", "[order_entry][json_decoder][dispatch]")
{
  auto decoder = make_decoder();
  const auto decoded = decoder.decode("not-json");
  CHECK_FALSE(decoded);
}
