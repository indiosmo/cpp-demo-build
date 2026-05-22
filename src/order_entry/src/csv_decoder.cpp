#include "order_entry/csv_decoder.hpp"

#include "order_entry/errors.hpp"

#include "lab/algorithm.hpp"
#include "lab/assert.hpp"
#include "lab/charconv.hpp"
#include "lab/error.hpp"
#include "lab/fmt.hpp"

#include <exception>
#include <string_view>

namespace order_entry {
namespace csv_decoder_detail {

namespace {

template <typename T>
T parse_number(std::string_view field)
{
  return lab::from_chars<T>(field).value();
}

} // namespace

types::side parse_side(std::string_view field)
{
  LAB_ASSERT(field == "B" || field == "S");

  if (field == "B") {
    return types::side::buy;
  } else if (field == "S") {
    return types::side::sell;
  } else {
    std::terminate();
  }
}

types::symbol parse_symbol(std::string_view field)
{
  return types::symbol::from(field).value();
}

namespace {

types::security_exchange default_security_exchange()
{
  return types::security_exchange{"BVMF"};
}

types::security_id demo_security_id(types::symbol)
{
  return types::security_id{0};
}

types::ord_type ord_type_from_price(types::price price)
{
  return price == 0 ? types::ord_type::market : types::ord_type::limit;
}

types::time_in_force time_in_force_from_price(types::price price)
{
  return price == 0 ? types::time_in_force::ioc : types::time_in_force::day;
}

} // namespace

new_order_single decode_new_order(std::string_view payload)
{
  const auto fields = lab::split_fields<6>(payload);
  const auto price = parse_number<types::price>(fields[2]);
  const auto symbol = parse_symbol(fields[1]);

  return new_order_single{
    .client_id = parse_number<types::client_id>(fields[0]),
    .cl_ord_id = parse_number<types::cl_ord_id>(fields[5]),
    .security_id = demo_security_id(symbol),
    .symbol = symbol,
    .security_exchange = default_security_exchange(),
    .side = parse_side(fields[4]),
    .ord_type = ord_type_from_price(price),
    .time_in_force = time_in_force_from_price(price),
    .order_qty = parse_number<types::quantity>(fields[3]),
    .price = price,
  };
}

cancel_order decode_cancel_order(std::string_view payload)
{
  const auto fields = lab::split_fields<2>(payload);
  const auto cl_ord_id = parse_number<types::cl_ord_id>(fields[1]);

  return cancel_order{
    .client_id = parse_number<types::client_id>(fields[0]),
    .cl_ord_id = cl_ord_id,
    .orig_cl_ord_id = types::orig_cl_ord_id{cl_ord_id.get()},
  };
}

replace_order decode_replace_order(std::string_view payload)
{
  const auto fields = lab::split_fields<7>(payload);
  const auto price = parse_number<types::price>(fields[2]);
  const auto symbol = parse_symbol(fields[1]);

  return replace_order{
    .client_id = parse_number<types::client_id>(fields[0]),
    .cl_ord_id = parse_number<types::cl_ord_id>(fields[5]),
    .orig_cl_ord_id = parse_number<types::orig_cl_ord_id>(fields[6]),
    .security_id = demo_security_id(symbol),
    .symbol = symbol,
    .security_exchange = default_security_exchange(),
    .side = parse_side(fields[4]),
    .ord_type = ord_type_from_price(price),
    .time_in_force = time_in_force_from_price(price),
    .order_qty = parse_number<types::quantity>(fields[3]),
    .price = price,
  };
}

flush decode_flush(std::string_view payload)
{
  LAB_ASSERT(payload.empty());
  return flush{};
}

} // namespace csv_decoder_detail

lab::result<request> csv_decoder::decode(std::string_view payload) const
{
  const auto line = lab::trim(payload);
  if (line.empty()) {
    return lab::make_leaf_error(errors::parser_error{.reason = "empty payload"});
  }

  const char msg_type = line.front();

  auto cursor = lab::trim(line.substr(1));

  if (cursor.starts_with(',')) {
    cursor.remove_prefix(1);
  }

  switch (msg_type) {
    case 'N':
      return csv_decoder_detail::decode_new_order(cursor);
    case 'R':
      return csv_decoder_detail::decode_replace_order(cursor);
    case 'C':
      return csv_decoder_detail::decode_cancel_order(cursor);
    case 'F':
      return csv_decoder_detail::decode_flush(cursor);
  }

  return lab::make_leaf_error(errors::parser_error{.reason = fmt::format("unknown message type '{}'", msg_type)});
}

} // namespace order_entry
