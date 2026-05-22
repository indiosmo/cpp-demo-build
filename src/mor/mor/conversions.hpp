#ifndef MOR_CONVERSIONS_HPP
#define MOR_CONVERSIONS_HPP

#include "lab/variant.hpp"
#include "mor/messages.hpp"
#include "order_entry/messages.hpp"

namespace mor {

[[nodiscard]] new_order_single to_mor(const order_entry::new_order_single& request);
[[nodiscard]] replace_request to_mor(const order_entry::replace_order& request);
[[nodiscard]] cancel_request to_mor(const order_entry::cancel_order& request);
[[nodiscard]] flush_request to_mor(const order_entry::flush& request);
[[nodiscard]] request to_mor(const order_entry::request& request);

[[nodiscard]] execution_report to_mor(const order_entry::execution_report& event);
[[nodiscard]] cancel_reject to_mor(const order_entry::cancel_reject& event);
[[nodiscard]] parser_reject to_mor(const order_entry::rejection& rejection);
[[nodiscard]] event to_mor(const order_entry::event& event);

[[nodiscard]] order_entry::new_order_single to_order_entry(const new_order_single& request);
[[nodiscard]] order_entry::replace_order to_order_entry(const replace_request& request);
[[nodiscard]] order_entry::cancel_order to_order_entry(const cancel_request& request);
[[nodiscard]] order_entry::flush to_order_entry(const flush_request& request);
[[nodiscard]] order_entry::request to_order_entry(const request& request);

[[nodiscard]] order_entry::execution_report to_order_entry(const execution_report& event);
[[nodiscard]] order_entry::cancel_reject to_order_entry(const cancel_reject& event);
[[nodiscard]] order_entry::rejection to_order_entry(const parser_reject& rejection);

} // namespace mor

#endif /* MOR_CONVERSIONS_HPP */
