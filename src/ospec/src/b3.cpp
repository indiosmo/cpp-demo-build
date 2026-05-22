#include "ospec/b3.hpp"

#include <cstdlib>

namespace ospec::b3 {

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
