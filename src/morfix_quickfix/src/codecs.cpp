#include "morfix_quickfix/codecs.hpp"

#include "lab/charconv.hpp"
#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/fmt.hpp"
#include "lab/strong_type.hpp"
#include "lab/variant.hpp"
#include "morfix_quickfix/errors.hpp"
#include "ospec/b3.hpp"

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

namespace morfix_quickfix::codecs::b3 {
namespace {

namespace spec = ospec::b3;

constexpr std::string_view new_order_single_msg_type = "D";
constexpr std::string_view order_cancel_replace_request_msg_type = "G";
constexpr std::string_view order_cancel_request_msg_type = "F";
constexpr std::string_view execution_report_msg_type = "8";
constexpr std::string_view order_cancel_reject_msg_type = "9";

template <typename T>
concept IntegralStrongType = lab::is_strong_type_v<T> && std::integral<typename T::UnderlyingType>;

template <typename T>
concept FixedStringStrongType =
  lab::is_strong_type_v<T> && requires (std::string_view value) {
    { T::from(value) } -> lab::Result;
  };

std::string as_field_value(const auto& value)
{
  return fmt::format("{}", value);
}

std::string as_field_value(char value)
{
  return std::string(1, value);
}

void set_field(quickfix_fix::message& message, spec::field_spec field, auto value)
{
  message.set(field.number, as_field_value(value));
}

void set_optional_field(quickfix_fix::message& message, spec::field_spec field, const auto& value)
{
  if (value) {
    set_field(message, field, *value);
  }
}

lab::result<std::string_view> get_required_view(const quickfix_fix::message& message, spec::field_spec field)
{
  if (const auto value = message.get(field.number)) {
    if (value->empty()) {
      return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("{} is empty", field.name));
    }
    return *value;
  }

  return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("missing required field {}", field.name));
}

template <IntegralStrongType T>
lab::result<T> parse_field_value(std::string_view value)
{
  return lab::from_chars<T>(value);
}

template <std::integral T>
lab::result<T> parse_field_value(std::string_view value)
{
  return lab::from_chars<T>(value);
}

template <FixedStringStrongType T>
lab::result<T> parse_field_value(std::string_view value)
{
  BOOST_LEAF_ASSIGN(auto parsed, T::from(value));
  return parsed;
}

template <typename T>
lab::result<T> get_required(const quickfix_fix::message& message, spec::field_spec field)
{
  BOOST_LEAF_ASSIGN(auto value, get_required_view(message, field));
  return parse_field_value<T>(value);
}

template <>
lab::result<std::string> get_required<std::string>(const quickfix_fix::message& message, spec::field_spec field)
{
  BOOST_LEAF_ASSIGN(auto value, get_required_view(message, field));
  return std::string{value};
}

template <>
lab::result<char> get_required<char>(const quickfix_fix::message& message, spec::field_spec field)
{
  BOOST_LEAF_ASSIGN(auto value, get_required_view(message, field));
  if (value.size() != 1) {
    return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("{} must be one character", field.name));
  }
  return value.front();
}

template <typename T>
lab::result<std::optional<T>> get_optional(const quickfix_fix::message& message, spec::field_spec field)
{
  if (!message.get(field.number)) {
    return std::nullopt;
  }

  BOOST_LEAF_ASSIGN(auto value, get_required<T>(message, field));
  return value;
}

lab::result<morfix::new_order_single> decode_new_order_single(const quickfix_fix::message& message)
{
  BOOST_LEAF_ASSIGN(auto side_value, get_required<char>(message, spec::side));
  BOOST_LEAF_ASSIGN(auto side, spec::parse_side(side_value));
  BOOST_LEAF_ASSIGN(auto ord_type_value, get_required<char>(message, spec::ord_type));
  BOOST_LEAF_ASSIGN(auto ord_type, spec::parse_ord_type(ord_type_value));
  BOOST_LEAF_ASSIGN(auto time_in_force_value, get_required<char>(message, spec::time_in_force));
  BOOST_LEAF_ASSIGN(auto time_in_force, spec::parse_time_in_force(time_in_force_value));
  BOOST_LEAF_ASSIGN(auto account, get_required<morfix::types::client_id>(message, spec::account));
  BOOST_LEAF_ASSIGN(auto cl_ord_id, get_required<morfix::types::cl_ord_id>(message, spec::cl_ord_id));
  BOOST_LEAF_ASSIGN(auto security_id, get_required<morfix::types::security_id>(message, spec::security_id));
  BOOST_LEAF_ASSIGN(auto symbol, get_required<morfix::types::symbol>(message, spec::symbol));
  BOOST_LEAF_ASSIGN(auto security_exchange, get_required<morfix::types::security_exchange>(message, spec::security_exchange));
  BOOST_LEAF_ASSIGN(auto order_qty, get_required<morfix::types::quantity>(message, spec::order_qty));
  BOOST_LEAF_ASSIGN(auto price, get_required<morfix::types::price>(message, spec::price));

  return morfix::new_order_single{
    .account = account,
    .cl_ord_id = cl_ord_id,
    .security_id = security_id,
    .symbol = symbol,
    .security_exchange = security_exchange,
    .side = side,
    .ord_type = ord_type,
    .time_in_force = time_in_force,
    .order_qty = order_qty,
    .price = price,
  };
}

lab::result<morfix::order_cancel_replace_request> decode_order_cancel_replace_request(const quickfix_fix::message& message)
{
  BOOST_LEAF_ASSIGN(auto order, decode_new_order_single(message));
  BOOST_LEAF_ASSIGN(auto orig_cl_ord_id, get_required<morfix::types::orig_cl_ord_id>(message, spec::orig_cl_ord_id));
  return morfix::order_cancel_replace_request{
    .account = order.account,
    .cl_ord_id = order.cl_ord_id,
    .orig_cl_ord_id = orig_cl_ord_id,
    .security_id = order.security_id,
    .symbol = order.symbol,
    .security_exchange = order.security_exchange,
    .side = order.side,
    .ord_type = order.ord_type,
    .time_in_force = order.time_in_force,
    .order_qty = order.order_qty,
    .price = order.price,
  };
}

lab::result<morfix::order_cancel_request> decode_order_cancel_request(const quickfix_fix::message& message)
{
  BOOST_LEAF_ASSIGN(auto account, get_required<morfix::types::client_id>(message, spec::account));
  BOOST_LEAF_ASSIGN(auto cl_ord_id, get_required<morfix::types::cl_ord_id>(message, spec::cl_ord_id));
  BOOST_LEAF_ASSIGN(auto orig_cl_ord_id, get_required<morfix::types::orig_cl_ord_id>(message, spec::orig_cl_ord_id));

  return morfix::order_cancel_request{
    .account = account,
    .cl_ord_id = cl_ord_id,
    .orig_cl_ord_id = orig_cl_ord_id,
  };
}

lab::result<morfix::execution_report> decode_execution_report(const quickfix_fix::message& message)
{
  BOOST_LEAF_ASSIGN(auto side_value, get_required<char>(message, spec::side));
  BOOST_LEAF_ASSIGN(auto side, spec::parse_side(side_value));
  BOOST_LEAF_ASSIGN(auto ord_type_value, get_required<char>(message, spec::ord_type));
  BOOST_LEAF_ASSIGN(auto ord_type, spec::parse_ord_type(ord_type_value));
  BOOST_LEAF_ASSIGN(auto time_in_force_value, get_required<char>(message, spec::time_in_force));
  BOOST_LEAF_ASSIGN(auto time_in_force, spec::parse_time_in_force(time_in_force_value));
  BOOST_LEAF_ASSIGN(auto exec_type_value, get_required<char>(message, spec::exec_type));
  BOOST_LEAF_ASSIGN(auto exec_type, spec::parse_exec_type(exec_type_value));
  BOOST_LEAF_ASSIGN(auto ord_status_value, get_required<char>(message, spec::ord_status));
  BOOST_LEAF_ASSIGN(auto ord_status, spec::parse_ord_status(ord_status_value));
  BOOST_LEAF_ASSIGN(auto reject_reason_code, get_optional<std::uint16_t>(message, spec::cxl_rej_reason));
  BOOST_LEAF_ASSIGN(auto account, get_required<morfix::types::client_id>(message, spec::account));
  BOOST_LEAF_ASSIGN(auto cl_ord_id, get_required<morfix::types::cl_ord_id>(message, spec::cl_ord_id));
  BOOST_LEAF_ASSIGN(auto orig_cl_ord_id, get_optional<morfix::types::orig_cl_ord_id>(message, spec::orig_cl_ord_id));
  BOOST_LEAF_ASSIGN(auto order_id, get_required<morfix::types::order_id>(message, spec::order_id));
  BOOST_LEAF_ASSIGN(auto exec_id, get_required<morfix::types::exec_id>(message, spec::exec_id));
  BOOST_LEAF_ASSIGN(auto security_id, get_required<morfix::types::security_id>(message, spec::security_id));
  BOOST_LEAF_ASSIGN(auto symbol, get_required<morfix::types::symbol>(message, spec::symbol));
  BOOST_LEAF_ASSIGN(auto security_exchange, get_required<morfix::types::security_exchange>(message, spec::security_exchange));
  BOOST_LEAF_ASSIGN(auto order_qty, get_required<morfix::types::quantity>(message, spec::order_qty));
  BOOST_LEAF_ASSIGN(auto cum_qty, get_required<morfix::types::quantity>(message, spec::cum_qty));
  BOOST_LEAF_ASSIGN(auto leaves_qty, get_required<morfix::types::quantity>(message, spec::leaves_qty));
  BOOST_LEAF_ASSIGN(auto last_qty, get_optional<morfix::types::quantity>(message, spec::last_qty));
  BOOST_LEAF_ASSIGN(auto last_px, get_optional<morfix::types::price>(message, spec::last_px));
  BOOST_LEAF_ASSIGN(auto avg_px, get_optional<morfix::types::price>(message, spec::avg_px));
  BOOST_LEAF_ASSIGN(auto transact_time, get_required<morfix::types::timestamp>(message, spec::transact_time));
  BOOST_LEAF_ASSIGN(auto text, get_optional<std::string>(message, spec::text));

  std::optional<morfix::types::reject_reason> reject_reason;
  if (reject_reason_code) {
    BOOST_LEAF_ASSIGN(reject_reason, spec::parse_reject_reason(*reject_reason_code));
  }

  return morfix::execution_report{
    .account = account,
    .cl_ord_id = cl_ord_id,
    .orig_cl_ord_id = orig_cl_ord_id,
    .order_id = order_id,
    .exec_id = exec_id,
    .security_id = security_id,
    .symbol = symbol,
    .security_exchange = security_exchange,
    .side = side,
    .ord_type = ord_type,
    .time_in_force = time_in_force,
    .exec_type = exec_type,
    .ord_status = ord_status,
    .order_qty = order_qty,
    .cum_qty = cum_qty,
    .leaves_qty = leaves_qty,
    .last_qty = last_qty,
    .last_px = last_px,
    .avg_px = avg_px,
    .transact_time = transact_time,
    .reject_reason = reject_reason,
    .text = text.value_or(""),
  };
}

lab::result<morfix::order_cancel_reject> decode_order_cancel_reject(const quickfix_fix::message& message)
{
  BOOST_LEAF_ASSIGN(auto reject_reason_code, get_required<std::uint16_t>(message, spec::cxl_rej_reason));
  BOOST_LEAF_ASSIGN(auto reject_reason, spec::parse_reject_reason(reject_reason_code));
  BOOST_LEAF_ASSIGN(auto account, get_required<morfix::types::client_id>(message, spec::account));
  BOOST_LEAF_ASSIGN(auto cl_ord_id, get_required<morfix::types::cl_ord_id>(message, spec::cl_ord_id));
  BOOST_LEAF_ASSIGN(auto orig_cl_ord_id, get_required<morfix::types::orig_cl_ord_id>(message, spec::orig_cl_ord_id));
  BOOST_LEAF_ASSIGN(auto text, get_optional<std::string>(message, spec::text));
  BOOST_LEAF_ASSIGN(auto transact_time, get_required<morfix::types::timestamp>(message, spec::transact_time));

  return morfix::order_cancel_reject{
    .account = account,
    .cl_ord_id = cl_ord_id,
    .orig_cl_ord_id = orig_cl_ord_id,
    .reject_reason = reject_reason,
    .text = text.value_or(""),
    .transact_time = transact_time,
  };
}

quickfix_fix::message encode_new_order_single(const morfix::new_order_single& request)
{
  quickfix_fix::message message{std::string{new_order_single_msg_type}};
  set_field(message, spec::account, request.account);
  set_field(message, spec::cl_ord_id, request.cl_ord_id);
  set_field(message, spec::security_id, request.security_id);
  set_field(message, spec::symbol, request.symbol);
  set_field(message, spec::security_exchange, request.security_exchange);
  set_field(message, spec::side, spec::normalize(request.side));
  set_field(message, spec::ord_type, spec::normalize(request.ord_type));
  set_field(message, spec::time_in_force, spec::normalize(request.time_in_force));
  set_field(message, spec::order_qty, request.order_qty);
  set_field(message, spec::price, request.price);
  return message;
}

quickfix_fix::message encode_order_cancel_replace_request(const morfix::order_cancel_replace_request& request)
{
  quickfix_fix::message message{std::string{order_cancel_replace_request_msg_type}};
  set_field(message, spec::account, request.account);
  set_field(message, spec::cl_ord_id, request.cl_ord_id);
  set_field(message, spec::orig_cl_ord_id, request.orig_cl_ord_id);
  set_field(message, spec::security_id, request.security_id);
  set_field(message, spec::symbol, request.symbol);
  set_field(message, spec::security_exchange, request.security_exchange);
  set_field(message, spec::side, spec::normalize(request.side));
  set_field(message, spec::ord_type, spec::normalize(request.ord_type));
  set_field(message, spec::time_in_force, spec::normalize(request.time_in_force));
  set_field(message, spec::order_qty, request.order_qty);
  set_field(message, spec::price, request.price);
  return message;
}

quickfix_fix::message encode_order_cancel_request(const morfix::order_cancel_request& request)
{
  quickfix_fix::message message{std::string{order_cancel_request_msg_type}};
  set_field(message, spec::account, request.account);
  set_field(message, spec::cl_ord_id, request.cl_ord_id);
  set_field(message, spec::orig_cl_ord_id, request.orig_cl_ord_id);
  return message;
}

quickfix_fix::message encode_execution_report(const morfix::execution_report& event)
{
  quickfix_fix::message message{std::string{execution_report_msg_type}};
  set_field(message, spec::account, event.account);
  set_field(message, spec::cl_ord_id, event.cl_ord_id);
  set_optional_field(message, spec::orig_cl_ord_id, event.orig_cl_ord_id);
  set_field(message, spec::order_id, event.order_id);
  set_field(message, spec::exec_id, event.exec_id);
  set_field(message, spec::security_id, event.security_id);
  set_field(message, spec::symbol, event.symbol);
  set_field(message, spec::security_exchange, event.security_exchange);
  set_field(message, spec::side, spec::normalize(event.side));
  set_field(message, spec::ord_type, spec::normalize(event.ord_type));
  set_field(message, spec::time_in_force, spec::normalize(event.time_in_force));
  set_field(message, spec::exec_type, spec::normalize(event.exec_type));
  set_field(message, spec::ord_status, spec::normalize(event.ord_status));
  set_field(message, spec::order_qty, event.order_qty);
  set_field(message, spec::cum_qty, event.cum_qty);
  set_field(message, spec::leaves_qty, event.leaves_qty);
  set_optional_field(message, spec::last_qty, event.last_qty);
  set_optional_field(message, spec::last_px, event.last_px);
  set_optional_field(message, spec::avg_px, event.avg_px);
  set_field(message, spec::transact_time, event.transact_time);
  if (event.reject_reason) {
    set_field(message, spec::cxl_rej_reason, spec::normalize(*event.reject_reason));
  }
  if (!event.text.empty()) {
    set_field(message, spec::text, event.text);
  }
  return message;
}

quickfix_fix::message encode_order_cancel_reject(const morfix::order_cancel_reject& event)
{
  quickfix_fix::message message{std::string{order_cancel_reject_msg_type}};
  set_field(message, spec::account, event.account);
  set_field(message, spec::cl_ord_id, event.cl_ord_id);
  set_field(message, spec::orig_cl_ord_id, event.orig_cl_ord_id);
  set_field(message, spec::cxl_rej_reason, spec::normalize(event.reject_reason));
  set_field(message, spec::transact_time, event.transact_time);
  if (!event.text.empty()) {
    set_field(message, spec::text, event.text);
  }
  return message;
}

} // namespace

lab::result<quickfix_fix::message> initiator::encode(const morfix::request& request) const
{
  return lab::match(
    request,
    [](const morfix::new_order_single& fix_request) { return encode_new_order_single(fix_request); },
    [](const morfix::order_cancel_replace_request& fix_request) { return encode_order_cancel_replace_request(fix_request); },
    [](const morfix::order_cancel_request& fix_request) { return encode_order_cancel_request(fix_request); });
}

lab::result<morfix::event> initiator::decode(const quickfix_fix::message& message) const
{
  if (message.msg_type() == execution_report_msg_type) {
    BOOST_LEAF_ASSIGN(auto event, decode_execution_report(message));
    return event;
  }
  if (message.msg_type() == order_cancel_reject_msg_type) {
    BOOST_LEAF_ASSIGN(auto event, decode_order_cancel_reject(message));
    return event;
  }

  return lab::make_leaf_error(errors::unsupported_message{.message_type = std::string{message.msg_type()}});
}

lab::result<quickfix_fix::message> acceptor::encode(const morfix::event& event) const
{
  return lab::match(
    event,
    [](const morfix::execution_report& fix_event) { return encode_execution_report(fix_event); },
    [](const morfix::order_cancel_reject& fix_event) { return encode_order_cancel_reject(fix_event); });
}

lab::result<morfix::request> acceptor::decode(const quickfix_fix::message& message) const
{
  if (message.msg_type() == new_order_single_msg_type) {
    BOOST_LEAF_ASSIGN(auto request, decode_new_order_single(message));
    return request;
  }
  if (message.msg_type() == order_cancel_replace_request_msg_type) {
    BOOST_LEAF_ASSIGN(auto request, decode_order_cancel_replace_request(message));
    return request;
  }
  if (message.msg_type() == order_cancel_request_msg_type) {
    BOOST_LEAF_ASSIGN(auto request, decode_order_cancel_request(message));
    return request;
  }

  return lab::make_leaf_error(errors::unsupported_message{.message_type = std::string{message.msg_type()}});
}

} // namespace morfix_quickfix::codecs::b3
