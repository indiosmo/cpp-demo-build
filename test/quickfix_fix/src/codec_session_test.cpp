#include "catch2/catch_test_macros.hpp"
#include "quickfix_fix/codec.hpp"
#include "quickfix_fix/message.hpp"
#include "quickfix_fix/session.hpp"

TEST_CASE("quickfix_fix message - replaces existing field in place", "[quickfix_fix][message]")
{
  quickfix_fix::message message{"D"};
  message.set(11, "100");
  message.set(55, "PETR4");
  message.set(11, "101");

  REQUIRE(message.fields().size() == 2);
  REQUIRE(message.get(11).has_value());
  CHECK(*message.get(11) == "101");
  CHECK(message.fields().front().number == 11);
}

TEST_CASE("quickfix_fix codec - encodes and decodes text records", "[quickfix_fix][codec]")
{
  quickfix_fix::message message{"D"};
  message.set(11, "42");
  message.set(55, "PETR4");

  const auto payload = quickfix_fix::encode(message, quickfix_fix::printable_delimiter);
  CHECK(payload == "35=D|11=42|55=PETR4|");

  auto decoded_result = quickfix_fix::decode(payload, quickfix_fix::printable_delimiter);
  REQUIRE(decoded_result);

  const auto decoded = decoded_result.value();
  CHECK(decoded.msg_type() == "D");
  REQUIRE(decoded.get(11).has_value());
  REQUIRE(decoded.get(55).has_value());
  CHECK(*decoded.get(11) == "42");
  CHECK(*decoded.get(55) == "PETR4");
}

TEST_CASE("quickfix_fix session - delivers messages between connected peers", "[quickfix_fix][session]")
{
  quickfix_fix::session_pair sessions;
  quickfix_fix::message inbound{"D"};

  sessions.acceptor.on_message = [&inbound](const quickfix_fix::message& message) { inbound = message; };

  quickfix_fix::message outbound{"F"};
  outbound.set(11, "77");
  const auto send_result = sessions.initiator.send(outbound);

  REQUIRE(send_result);
  CHECK(inbound.msg_type() == "F");
  REQUIRE(inbound.get(11).has_value());
  CHECK(*inbound.get(11) == "77");
}
