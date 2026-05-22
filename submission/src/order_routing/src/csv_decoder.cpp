#include "order_routing/csv_decoder.hpp"

#include "order_routing/errors.hpp"

#include "kraken/algorithm.hpp"
#include "kraken/assert.hpp"
#include "kraken/charconv.hpp"
#include "kraken/error.hpp"
#include "kraken/fmt.hpp"

#include <exception>
#include <string_view>

namespace order_routing {
namespace csv_decoder_detail {

namespace {

template <typename T>
T parse_number(std::string_view field)
{
  return kraken::from_chars<T>(field).value();
}

} // namespace

types::side parse_side(std::string_view field)
{
  KRAKEN_ASSERT(field == "B" || field == "S");

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

new_order decode_new_order(std::string_view payload)
{
  const auto fields = kraken::split_fields<6>(payload);

  return new_order{
    .user = parse_number<types::user_id>(fields[0]),
    .order_id = parse_number<types::user_order_id>(fields[5]),
    .instrument = parse_symbol(fields[1]),
    .order_side = parse_side(fields[4]),
    .limit_price = parse_number<types::price>(fields[2]),
    .order_quantity = parse_number<types::quantity>(fields[3]),
  };
}

cancel_order decode_cancel_order(std::string_view payload)
{
  const auto fields = kraken::split_fields<2>(payload);

  return cancel_order{
    .user = parse_number<types::user_id>(fields[0]),
    .order_id = parse_number<types::user_order_id>(fields[1]),
  };
}

flush decode_flush(std::string_view payload)
{
  KRAKEN_ASSERT(payload.empty());
  return flush{};
}

} // namespace csv_decoder_detail

kraken::result<request> csv_decoder::decode(std::string_view payload) const
{
  const auto line = kraken::trim(payload);
  if (line.empty()) {
    return kraken::make_leaf_error(errors::parser_error{.reason = "empty payload"});
  }

  const char msg_type = line.front();

  auto cursor = kraken::trim(line.substr(1));

  if (cursor.starts_with(',')) {
    cursor.remove_prefix(1);
  }

  switch (msg_type) {
    case 'N':
      return csv_decoder_detail::decode_new_order(cursor);
    case 'C':
      return csv_decoder_detail::decode_cancel_order(cursor);
    case 'F':
      return csv_decoder_detail::decode_flush(cursor);
  }

  return kraken::make_leaf_error(errors::parser_error{.reason = fmt::format("unknown message type '{}'", msg_type)});
}

} // namespace order_routing
