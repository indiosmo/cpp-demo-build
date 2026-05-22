#ifndef MORFIX_CONVERSIONS_HPP
#define MORFIX_CONVERSIONS_HPP

#include "mor/messages.hpp"
#include "morfix/messages.hpp"

#include <optional>

namespace morfix {

[[nodiscard]] new_order_single to_fix(const mor::new_order_single& request);
[[nodiscard]] order_cancel_replace_request to_fix(const mor::replace_request& request);
[[nodiscard]] order_cancel_request to_fix(const mor::cancel_request& request);
[[nodiscard]] std::optional<request> to_fix(const mor::request& request);

[[nodiscard]] execution_report to_fix(const mor::execution_report& event);
[[nodiscard]] order_cancel_reject to_fix(const mor::cancel_reject& event);
[[nodiscard]] std::optional<event> to_fix(const mor::event& event);

} // namespace morfix

#endif /* MORFIX_CONVERSIONS_HPP */
