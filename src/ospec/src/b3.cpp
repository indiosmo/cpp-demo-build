#include "ospec/b3.hpp"

#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/fmt.hpp"

#include <cstdlib>

namespace ospec::b3 {
namespace {

template <typename T>
lab::result<T> invalid_value(std::string_view field_name, auto value)
{
  return lab::make_leaf_error(lab::error_code::invalid_argument, fmt::format("invalid B3 {} value '{}'", field_name, value));
}

} // namespace

char normalize(mor::types::side value)
{
  switch (value) {
    case mor::types::side::buy:
      return '1';
    case mor::types::side::sell:
      return '2';
  }

  std::terminate();
}

char normalize(mor::types::ord_type value)
{
  switch (value) {
    case mor::types::ord_type::market:
      return '1';
    case mor::types::ord_type::limit:
      return '2';
  }

  std::terminate();
}

char normalize(mor::types::time_in_force value)
{
  switch (value) {
    case mor::types::time_in_force::day:
      return '0';
    case mor::types::time_in_force::ioc:
      return '3';
  }

  std::terminate();
}

char normalize(mor::types::exec_type value)
{
  switch (value) {
    case mor::types::exec_type::new_order:
      return '0';
    case mor::types::exec_type::replaced:
      return '5';
    case mor::types::exec_type::canceled:
      return '4';
    case mor::types::exec_type::trade:
      return 'F';
    case mor::types::exec_type::rejected:
      return '8';
    case mor::types::exec_type::expired:
      return 'C';
  }

  std::terminate();
}

char normalize(mor::types::ord_status value)
{
  switch (value) {
    case mor::types::ord_status::new_order:
      return '0';
    case mor::types::ord_status::partially_filled:
      return '1';
    case mor::types::ord_status::filled:
      return '2';
    case mor::types::ord_status::canceled:
      return '4';
    case mor::types::ord_status::replaced:
      return '5';
    case mor::types::ord_status::rejected:
      return '8';
    case mor::types::ord_status::expired:
      return 'C';
  }

  std::terminate();
}

std::uint16_t normalize(mor::types::reject_reason value)
{
  switch (value) {
    case mor::types::reject_reason::unknown_order:
      return 1;
    case mor::types::reject_reason::duplicate_order:
      return 6;
    case mor::types::reject_reason::unknown_symbol:
      return 1;
    case mor::types::reject_reason::unsupported_request:
      return 99;
  }

  std::terminate();
}

lab::result<mor::types::side> parse_side(char value)
{
  switch (value) {
    case '1':
      return mor::types::side::buy;
    case '2':
      return mor::types::side::sell;
    default:
      return invalid_value<mor::types::side>("Side", value);
  }
}

lab::result<mor::types::ord_type> parse_ord_type(char value)
{
  switch (value) {
    case '1':
      return mor::types::ord_type::market;
    case '2':
      return mor::types::ord_type::limit;
    default:
      return invalid_value<mor::types::ord_type>("OrdType", value);
  }
}

lab::result<mor::types::time_in_force> parse_time_in_force(char value)
{
  switch (value) {
    case '0':
      return mor::types::time_in_force::day;
    case '3':
      return mor::types::time_in_force::ioc;
    default:
      return invalid_value<mor::types::time_in_force>("TimeInForce", value);
  }
}

lab::result<mor::types::exec_type> parse_exec_type(char value)
{
  switch (value) {
    case '0':
      return mor::types::exec_type::new_order;
    case '5':
      return mor::types::exec_type::replaced;
    case '4':
      return mor::types::exec_type::canceled;
    case 'F':
      return mor::types::exec_type::trade;
    case '8':
      return mor::types::exec_type::rejected;
    case 'C':
      return mor::types::exec_type::expired;
    default:
      return invalid_value<mor::types::exec_type>("ExecType", value);
  }
}

lab::result<mor::types::ord_status> parse_ord_status(char value)
{
  switch (value) {
    case '0':
      return mor::types::ord_status::new_order;
    case '1':
      return mor::types::ord_status::partially_filled;
    case '2':
      return mor::types::ord_status::filled;
    case '4':
      return mor::types::ord_status::canceled;
    case '5':
      return mor::types::ord_status::replaced;
    case '8':
      return mor::types::ord_status::rejected;
    case 'C':
      return mor::types::ord_status::expired;
    default:
      return invalid_value<mor::types::ord_status>("OrdStatus", value);
  }
}

lab::result<mor::types::reject_reason> parse_reject_reason(std::uint16_t value)
{
  switch (value) {
    case 1:
      return mor::types::reject_reason::unknown_order;
    case 6:
      return mor::types::reject_reason::duplicate_order;
    case 99:
      return mor::types::reject_reason::unsupported_request;
    default:
      return invalid_value<mor::types::reject_reason>("RejectReason", value);
  }
}

char normalize(mmd::types::side value)
{
  switch (value) {
    case mmd::types::side::buy:
      return '0';
    case mmd::types::side::sell:
      return '1';
  }

  std::terminate();
}

char normalize(mmd::types::update_action value)
{
  switch (value) {
    case mmd::types::update_action::new_order:
      return '0';
    case mmd::types::update_action::change:
      return '1';
    case mmd::types::update_action::delete_order:
      return '2';
  }

  std::terminate();
}

char normalize(mmd::types::trade_condition value)
{
  switch (value) {
    case mmd::types::trade_condition::regular:
      return '0';
  }

  std::terminate();
}

} // namespace ospec::b3
