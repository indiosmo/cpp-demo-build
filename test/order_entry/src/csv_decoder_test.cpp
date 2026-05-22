#include "catch2/catch_test_macros.hpp"
#include "order_entry/csv_decoder.hpp"
#include "order_entry/messages.hpp"
#include "order_entry/types.hpp"

#include <string_view>
#include <variant>

/*
 * Component tests for order_entry::csv_decoder -- the wire-protocol parser
 * that maps one UDP datagram into one typed request. The grammar under test
 * (three fixed-arity request shapes, whitespace tolerance around tokens,
 * "B"/"S" side characters, market orders signalled by price zero) is
 * documented in docs/engine-specs.md.
 *
 * The well-formed grammar is a documented precondition: malformed payloads
 * trigger LAB_ASSERT and abort in debug builds, so these tests exercise
 * only the contract the implementation promises to honour. The per-record
 * decoders in csv_decoder_detail consume the post-marker field list (i.e.
 * the payload with the "N,"/"C,"/"F" prefix already stripped).
 */

namespace {

using namespace std::string_view_literals;

namespace rt = order_entry;
namespace detail = rt::csv_decoder_detail;

} // namespace

TEST_CASE("csv_decoder_detail - decode_new_order", "[order_entry][csv_decoder][parse]")
{
  const auto cmd = detail::decode_new_order(" 1, IBM, 10, 100, B, 1");

  CHECK(cmd.client_id == rt::types::client_id{1});
  CHECK(cmd.symbol == rt::types::symbol{"IBM"});
  CHECK(cmd.price == rt::types::price{10});
  CHECK(cmd.order_qty == rt::types::quantity{100});
  CHECK(cmd.side == rt::types::side::buy);
  CHECK(cmd.cl_ord_id == rt::types::cl_ord_id{1});
  CHECK(cmd.ord_type == rt::types::ord_type::limit);
  CHECK(cmd.time_in_force == rt::types::time_in_force::day);
}

TEST_CASE("csv_decoder_detail - decode_cancel_order", "[order_entry][csv_decoder][parse]")
{
  const auto cmd = detail::decode_cancel_order(" 7, 42");

  CHECK(cmd.client_id == rt::types::client_id{7});
  CHECK(cmd.cl_ord_id == rt::types::cl_ord_id{42});
  CHECK(cmd.orig_cl_ord_id == rt::types::orig_cl_ord_id{42});
}

TEST_CASE("csv_decoder_detail - parse_side", "[order_entry][csv_decoder][parse]")
{
  CHECK(detail::parse_side("B") == rt::types::side::buy);
  CHECK(detail::parse_side("S") == rt::types::side::sell);
}

TEST_CASE("csv_decoder - decode dispatches by marker", "[order_entry][csv_decoder][dispatch]")
{
  rt::csv_decoder decoder;

  const auto new_order = decoder.decode("N, 1, IBM, 10, 100, B, 1");
  REQUIRE(new_order);
  CHECK(std::holds_alternative<rt::new_order_single>(*new_order));

  const auto replace = decoder.decode("R, 1, IBM, 11, 90, B, 2, 1");
  REQUIRE(replace);
  CHECK(std::holds_alternative<rt::replace_order>(*replace));

  const auto cancel = decoder.decode("C, 7, 42");
  REQUIRE(cancel);
  CHECK(std::holds_alternative<rt::cancel_order>(*cancel));

  const auto flush = decoder.decode("F");
  REQUIRE(flush);
  CHECK(std::holds_alternative<rt::flush>(*flush));
}

TEST_CASE("csv_decoder - decode trims whitespace", "[order_entry][csv_decoder][dispatch]")
{
  rt::csv_decoder decoder;

  const auto decoded = decoder.decode("   N, 1, IBM, 10, 100, B, 1\r\n");
  REQUIRE(decoded);
  CHECK(std::holds_alternative<rt::new_order_single>(*decoded));
}
