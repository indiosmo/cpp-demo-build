#include "catch2/catch_test_macros.hpp"
#include "market_data/csv_encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/types.hpp"

#include <optional>
#include <string>

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

TEST_CASE("csv_encoder - encode execution_summary", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::execution_summary{
      .security_id = md::types::security_id{1},
      .aggressor_side = md::types::side::sell,
      .last_px = md::types::price{10},
      .fill_qty = md::types::quantity{100},
      .traded_hidden_qty = std::nullopt,
      .cancel_qty = std::nullopt,
      .aggressor_time = md::types::timestamp{0},
      .transact_time = md::types::timestamp{0},
    });

  CHECK(record == "E, 1, S, 10, 100, 0, 0");
}

TEST_CASE("csv_encoder - encode trade", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::trade{
      .security_id = md::types::security_id{1},
      .trade_id = md::types::trade_id{7},
      .price = md::types::price{11},
      .quantity = md::types::quantity{20},
      .buyer = md::types::order_id{3},
      .seller = md::types::order_id{102},
      .trade_condition = md::types::trade_condition::regular,
      .trade_sub_type = std::nullopt,
      .trade_date = md::types::trade_date{"20260522"},
      .transact_time = md::types::timestamp{0},
    });

  CHECK(record == "T, 1, 7, 11, 20, 3, 102, 0, 0");
}

TEST_CASE("csv_encoder - encode mbo_book_update", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::mbo_book_update{
      .security_id = md::types::security_id{1},
      .update_action = md::types::update_action::change,
      .side = md::types::side::buy,
      .resting_order_id = md::types::order_id{0},
      .price = md::types::price{10},
      .quantity = md::types::quantity{150},
      .previous_quantity = std::nullopt,
      .transact_time = md::types::timestamp{0},
    });

  CHECK(record == "M, 1, C, B, 0, 10, 150, -, 0");
}

TEST_CASE("csv_encoder - encode mbo_book_update delete", "[market_data][csv_encoder][format]")
{
  const auto record = encode_message(
    md::mbo_book_update{
      .security_id = md::types::security_id{1},
      .update_action = md::types::update_action::delete_order,
      .side = md::types::side::sell,
      .resting_order_id = md::types::order_id{0},
      .price = std::nullopt,
      .quantity = std::nullopt,
      .previous_quantity = std::nullopt,
      .transact_time = md::types::timestamp{0},
    });

  CHECK(record == "M, 1, D, S, 0, -, -, -, 0");
}

TEST_CASE("csv_encoder_detail - encode enum tokens", "[market_data][csv_encoder][format]")
{
  CHECK(detail::encode_side(md::types::side::buy) == 'B');
  CHECK(detail::encode_side(md::types::side::sell) == 'S');
  CHECK(detail::encode_update_action(md::types::update_action::new_order) == 'N');
  CHECK(detail::encode_update_action(md::types::update_action::change) == 'C');
  CHECK(detail::encode_update_action(md::types::update_action::delete_order) == 'D');
}
