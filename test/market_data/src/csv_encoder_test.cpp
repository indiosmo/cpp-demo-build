#include "catch2/catch_test_macros.hpp"
#include "market_data/csv_encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/types.hpp"

#include <cstdint>
#include <optional>
#include <string>

/*
 * Component tests for market_data::csv_encoder -- the wire-protocol formatter
 * that maps one typed market-data message into one stdout CSV record. The
 * expected records follow the outbound CSV protocol: comma-space separated
 * fields, "B"/"S" side tokens, and "-" for an eliminated top-of-book side.
 */

namespace {

namespace md = market_data;

namespace detail = md::csv_encoder_detail;

template <typename Message>
std::string encode_message(const Message& msg)
{
  const md::csv_encoder encoder;
  return encoder.encode(md::message{msg});
}

} // namespace

TEST_CASE("csv_encoder - encode order_ack", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::order_ack{
      .user = md::types::user_id{1},
      .order_id = md::types::user_order_id{2},
    });

  CHECK(record == "A, 1, 2");
}

TEST_CASE("csv_encoder - encode cancel_ack", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::cancel_ack{
      .user = md::types::user_id{7},
      .order_id = md::types::user_order_id{42},
    });

  CHECK(record == "C, 7, 42");
}

TEST_CASE("csv_encoder - encode trade", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::trade{
      .buy_user = md::types::user_id{1},
      .buy_order = md::types::user_order_id{3},
      .sell_user = md::types::user_id{2},
      .sell_order = md::types::user_order_id{102},
      .trade_price = md::types::price{11},
      .trade_quantity = md::types::quantity{20},
    });

  CHECK(record == "T, 1, 3, 2, 102, 11, 20");
}

TEST_CASE("csv_encoder - encode top_of_book buy side", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::top_of_book{
      .book_side = md::types::side::buy,
      .top_price = md::types::price{10},
      .top_quantity = md::types::total_quantity{150},
    });

  CHECK(record == "B, B, 10, 150");
}

TEST_CASE("csv_encoder - encode top_of_book sell side", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::top_of_book{
      .book_side = md::types::side::sell,
      .top_price = md::types::price{12},
      .top_quantity = md::types::total_quantity{100},
    });

  CHECK(record == "B, S, 12, 100");
}

TEST_CASE("csv_encoder - encode top_of_book empty side", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::top_of_book{
      .book_side = md::types::side::buy,
      .top_price = std::nullopt,
      .top_quantity = std::nullopt,
    });

  CHECK(record == "B, B, -, -");
}

TEST_CASE("csv_encoder_detail - encode_side", "[market_data][csv_encoder][format]")
{
  CHECK(detail::encode_side(md::types::side::buy) == 'B');
  CHECK(detail::encode_side(md::types::side::sell) == 'S');
}
