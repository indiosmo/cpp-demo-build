#include "order_entry/json_decoder.hpp"

#include "lab/fmt.hpp"
#include "order_entry/errors.hpp"

#include <exception>
#include <string>
#include <string_view>

namespace order_entry {
namespace json_decoder_detail {

namespace {

template <typename T>
T get_required(const lab::json::value& payload, std::string_view field_name)
{
  return payload.at(std::string{field_name}).get<T>();
}

template <typename T>
T get_optional(const lab::json::value& payload, std::string_view field_name, T fallback)
{
  if (auto it = payload.find(std::string{field_name}); it != payload.end()) {
    return it->get<T>();
  }
  return fallback;
}

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

types::side decode_side(std::string_view field)
{
  if (field == "buy") {
    return types::side::buy;
  }
  if (field == "sell") {
    return types::side::sell;
  }
  throw std::runtime_error{fmt::format("invalid side '{}'", field)};
}

types::ord_type decode_ord_type(std::string_view field)
{
  if (field == "market") {
    return types::ord_type::market;
  }
  if (field == "limit") {
    return types::ord_type::limit;
  }
  throw std::runtime_error{fmt::format("invalid ord_type '{}'", field)};
}

types::time_in_force decode_time_in_force(std::string_view field)
{
  if (field == "day") {
    return types::time_in_force::day;
  }
  if (field == "ioc") {
    return types::time_in_force::ioc;
  }
  throw std::runtime_error{fmt::format("invalid time_in_force '{}'", field)};
}

new_order_single decode_new_order(const lab::json::value& payload)
{
  const auto symbol = get_required<types::symbol>(payload, "symbol");
  const auto price = get_required<types::price>(payload, "price");

  return new_order_single{
    .client_id = get_required<types::client_id>(payload, "client_id"),
    .cl_ord_id = get_required<types::cl_ord_id>(payload, "cl_ord_id"),
    .security_id = get_optional(payload, "security_id", demo_security_id(symbol)),
    .symbol = symbol,
    .security_exchange = get_optional(payload, "security_exchange", default_security_exchange()),
    .side = decode_side(get_required<std::string>(payload, "side")),
    .ord_type =
      payload.contains("ord_type") ? decode_ord_type(get_required<std::string>(payload, "ord_type")) : ord_type_from_price(price),
    .time_in_force = payload.contains("time_in_force") ? decode_time_in_force(get_required<std::string>(payload, "time_in_force"))
                                                       : time_in_force_from_price(price),
    .order_qty = get_required<types::quantity>(payload, "order_qty"),
    .price = price,
  };
}

replace_order decode_replace_order(const lab::json::value& payload)
{
  const auto symbol = get_required<types::symbol>(payload, "symbol");
  const auto price = get_required<types::price>(payload, "price");

  return replace_order{
    .client_id = get_required<types::client_id>(payload, "client_id"),
    .cl_ord_id = get_required<types::cl_ord_id>(payload, "cl_ord_id"),
    .orig_cl_ord_id = get_required<types::orig_cl_ord_id>(payload, "orig_cl_ord_id"),
    .security_id = get_optional(payload, "security_id", demo_security_id(symbol)),
    .symbol = symbol,
    .security_exchange = get_optional(payload, "security_exchange", default_security_exchange()),
    .side = decode_side(get_required<std::string>(payload, "side")),
    .ord_type =
      payload.contains("ord_type") ? decode_ord_type(get_required<std::string>(payload, "ord_type")) : ord_type_from_price(price),
    .time_in_force = payload.contains("time_in_force") ? decode_time_in_force(get_required<std::string>(payload, "time_in_force"))
                                                       : time_in_force_from_price(price),
    .order_qty = get_required<types::quantity>(payload, "order_qty"),
    .price = price,
  };
}

cancel_order decode_cancel_order(const lab::json::value& payload)
{
  return cancel_order{
    .client_id = get_required<types::client_id>(payload, "client_id"),
    .cl_ord_id = get_required<types::cl_ord_id>(payload, "cl_ord_id"),
    .orig_cl_ord_id = get_required<types::orig_cl_ord_id>(payload, "orig_cl_ord_id"),
  };
}

flush decode_flush(const lab::json::value&)
{
  return flush{};
}

} // namespace json_decoder_detail

json_decoder::json_decoder(json_decoder_config config)
  : config_{config}
{
}

lab::result<request> json_decoder::decode(std::string_view payload) const
{
  if (payload.size() > config_.max_datagram_size) {
    return lab::make_leaf_error(errors::parser_error{.reason = "datagram exceeds configured maximum size"});
  }

  try {
    const auto document = lab::json::value::parse(payload.begin(), payload.end());
    const auto message_type = document.at("message_type").get<std::string>();

    if (message_type == "new_order_single") {
      return json_decoder_detail::decode_new_order(document);
    }
    if (message_type == "replace_order") {
      return json_decoder_detail::decode_replace_order(document);
    }
    if (message_type == "cancel_order") {
      return json_decoder_detail::decode_cancel_order(document);
    }
    if (message_type == "flush") {
      return json_decoder_detail::decode_flush(document);
    }

    return lab::make_leaf_error(errors::parser_error{.reason = fmt::format("unknown message_type '{}'", message_type)});
  } catch (const std::exception& ex) {
    return lab::make_leaf_error(errors::parser_error{.reason = fmt::format("json decode failed: {}", ex.what())});
  }
}

} // namespace order_entry
