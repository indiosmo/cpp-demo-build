#include "catch2/catch_test_macros.hpp"
#include "order_routing/csv_decoder.hpp"
#include "order_routing/messages.hpp"
#include "order_routing/types.hpp"

#include <string_view>
#include <variant>

/*
 * Component tests for order_routing::csv_decoder -- the wire-protocol parser
 * that maps one UDP datagram into one typed request. The grammar under test
 * (three fixed-arity request shapes, whitespace tolerance around tokens,
 * "B"/"S" side characters, market orders signalled by price zero) is the one
 * refined by ADR 0003.
 *
 * The well-formed grammar is a documented precondition: malformed payloads
 * trigger LAB_ASSERT and abort in debug builds, so these tests exercise
 * only the contract the implementation promises to honour. The per-record
 * decoders in csv_decoder_detail consume the post-marker field list (i.e.
 * the payload with the "N,"/"C,"/"F" prefix already stripped).
 */

namespace {

using namespace std::string_view_literals;

namespace rt = order_routing;
namespace detail = rt::csv_decoder_detail;

} // namespace

TEST_CASE("csv_decoder_detail - decode_new_order", "[order_routing][csv_decoder][parse]")
{
  const auto cmd = detail::decode_new_order(" 1, IBM, 10, 100, B, 1");

  CHECK(cmd.user == rt::types::user_id{1});
  CHECK(cmd.instrument == rt::types::symbol{"IBM"});
  CHECK(cmd.limit_price == rt::types::price{10});
  CHECK(cmd.order_quantity == rt::types::quantity{100});
  CHECK(cmd.order_side == rt::types::side::buy);
  CHECK(cmd.order_id == rt::types::user_order_id{1});
}

TEST_CASE("csv_decoder_detail - decode_cancel_order", "[order_routing][csv_decoder][parse]")
{
  const auto cmd = detail::decode_cancel_order(" 7, 42");

  CHECK(cmd.user == rt::types::user_id{7});
  CHECK(cmd.order_id == rt::types::user_order_id{42});
}

TEST_CASE("csv_decoder_detail - parse_side", "[order_routing][csv_decoder][parse]")
{
  CHECK(detail::parse_side("B") == rt::types::side::buy);
  CHECK(detail::parse_side("S") == rt::types::side::sell);
}

TEST_CASE("csv_decoder - decode dispatches by marker", "[order_routing][csv_decoder][dispatch]")
{
  rt::csv_decoder decoder;

  const auto new_order = decoder.decode("N, 1, IBM, 10, 100, B, 1");
  REQUIRE(new_order);
  CHECK(std::holds_alternative<rt::new_order>(*new_order));

  const auto cancel = decoder.decode("C, 7, 42");
  REQUIRE(cancel);
  CHECK(std::holds_alternative<rt::cancel_order>(*cancel));

  const auto flush = decoder.decode("F");
  REQUIRE(flush);
  CHECK(std::holds_alternative<rt::flush>(*flush));
}

TEST_CASE("csv_decoder - decode trims whitespace", "[order_routing][csv_decoder][dispatch]")
{
  rt::csv_decoder decoder;

  const auto decoded = decoder.decode("   N, 1, IBM, 10, 100, B, 1\r\n");
  REQUIRE(decoded);
  CHECK(std::holds_alternative<rt::new_order>(*decoded));
}
