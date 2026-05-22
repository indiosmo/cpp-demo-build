#include "catch2/catch_test_macros.hpp"
#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "morfix/conversions.hpp"
#include "morfix_quickfix/codecs.hpp"
#include "morfix_quickfix/error_code.hpp"
#include "ospec/b3.hpp"
#include "quickfix_fix/session.hpp"

#include "boost/leaf/handle_errors.hpp"

#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace {

morfix::new_order_single make_new_order()
{
  return morfix::new_order_single{
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
}

morfix::execution_report make_execution_report()
{
  return morfix::execution_report{
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
}

template <typename Fn>
std::error_code capture_error_code(Fn&& fn)
{
  return boost::leaf::try_handle_all(
           [&]() -> lab::result<std::optional<std::error_code>> {
             auto result = std::forward<Fn>(fn)();
             if (result) {
               return std::nullopt;
             }

             return result.error();
           },
           [](const lab::error& err) { return std::optional{err.error_code()}; },
           [](const std::error_code& ec) { return std::optional{ec}; },
           [] { return std::optional<std::error_code>{}; })
    .value_or(std::error_code{});
}

} // namespace

TEST_CASE("morfix_quickfix b3 initiator - encodes new order request", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::initiator codec;

  auto encoded = codec.encode(morfix::request{make_new_order()});
  REQUIRE(encoded);

  const auto message = encoded.value();
  CHECK(message.msg_type() == "D");
  REQUIRE(message.get(ospec::b3::account.number).has_value());
  REQUIRE(message.get(ospec::b3::side.number).has_value());
  REQUIRE(message.get(ospec::b3::ord_type.number).has_value());
  CHECK(*message.get(ospec::b3::account.number) == "7");
  CHECK(*message.get(ospec::b3::side.number) == "1");
  CHECK(*message.get(ospec::b3::ord_type.number) == "2");
}

TEST_CASE("morfix_quickfix b3 acceptor - decodes new order request", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::initiator initiator_codec;
  const morfix_quickfix::codecs::b3::acceptor acceptor_codec;

  auto encoded = initiator_codec.encode(morfix::request{make_new_order()});
  REQUIRE(encoded);

  auto decoded = acceptor_codec.decode(encoded.value());
  REQUIRE(decoded);

  REQUIRE(std::holds_alternative<morfix::new_order_single>(decoded.value()));
  const auto& order = std::get<morfix::new_order_single>(decoded.value());
  CHECK(order.account == morfix::types::client_id{7});
  CHECK(order.cl_ord_id == morfix::types::cl_ord_id{42});
  CHECK(order.symbol == morfix::types::symbol{"PETR4"});
  CHECK(order.price == morfix::types::price{2750});
}

TEST_CASE("morfix_quickfix b3 acceptor - encodes execution report", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::acceptor codec;

  auto encoded = codec.encode(morfix::event{make_execution_report()});
  REQUIRE(encoded);

  const auto message = encoded.value();
  CHECK(message.msg_type() == "8");
  REQUIRE(message.get(ospec::b3::exec_type.number).has_value());
  REQUIRE(message.get(ospec::b3::ord_status.number).has_value());
  REQUIRE(message.get(ospec::b3::text.number).has_value());
  CHECK(*message.get(ospec::b3::exec_type.number) == "0");
  CHECK(*message.get(ospec::b3::ord_status.number) == "0");
  CHECK(*message.get(ospec::b3::text.number) == "accepted");
}

TEST_CASE("morfix_quickfix b3 initiator - decodes execution report", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::acceptor acceptor_codec;
  const morfix_quickfix::codecs::b3::initiator initiator_codec;

  auto encoded = acceptor_codec.encode(morfix::event{make_execution_report()});
  REQUIRE(encoded);

  auto decoded = initiator_codec.decode(encoded.value());
  REQUIRE(decoded);

  REQUIRE(std::holds_alternative<morfix::execution_report>(decoded.value()));
  const auto& report = std::get<morfix::execution_report>(decoded.value());
  CHECK(report.account == morfix::types::client_id{7});
  CHECK(report.order_id == morfix::types::order_id{1001});
  CHECK(report.leaves_qty == morfix::types::quantity{100});
  CHECK(report.text == "accepted");
}

TEST_CASE("morfix_quickfix b3 - local initiator acceptor loop carries request and event", "[morfix_quickfix][session]")
{
  quickfix_fix::session_pair sessions;
  const morfix_quickfix::codecs::b3::initiator initiator_codec;
  const morfix_quickfix::codecs::b3::acceptor acceptor_codec;
  std::optional<morfix::request> server_request;
  std::optional<morfix::event> client_event;

  sessions.acceptor.on_message = [&](const quickfix_fix::message& message) {
    auto decoded = acceptor_codec.decode(message);
    REQUIRE(decoded);
    server_request = decoded.value();

    auto encoded = acceptor_codec.encode(morfix::event{make_execution_report()});
    REQUIRE(encoded);
    const auto send_result = sessions.acceptor.send(encoded.value());
    REQUIRE(send_result);
  };

  sessions.initiator.on_message = [&](const quickfix_fix::message& message) {
    auto decoded = initiator_codec.decode(message);
    REQUIRE(decoded);
    client_event = decoded.value();
  };

  auto encoded = initiator_codec.encode(morfix::request{make_new_order()});
  REQUIRE(encoded);
  const auto send_result = sessions.initiator.send(encoded.value());
  REQUIRE(send_result);

  REQUIRE(server_request.has_value());
  REQUIRE(std::holds_alternative<morfix::new_order_single>(*server_request));
  CHECK(std::get<morfix::new_order_single>(*server_request).cl_ord_id == morfix::types::cl_ord_id{42});

  REQUIRE(client_event.has_value());
  REQUIRE(std::holds_alternative<morfix::execution_report>(*client_event));
  CHECK(std::get<morfix::execution_report>(*client_event).ord_status == morfix::types::ord_status::new_order);
}

TEST_CASE("morfix_quickfix b3 acceptor - unknown FIX message returns typed failure", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::acceptor codec;
  const quickfix_fix::message message{"Z"};

  const auto ec = capture_error_code([&] { return codec.decode(message); });
  CHECK(ec == morfix_quickfix::error_code::unsupported_message);
}

TEST_CASE("morfix_quickfix b3 acceptor - missing required field returns typed failure", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::acceptor codec;
  quickfix_fix::message message{"D"};
  message.set(ospec::b3::cl_ord_id.number, "42");

  const auto ec = capture_error_code([&] { return codec.decode(message); });
  CHECK(ec == lab::error_code::invalid_argument);
}
