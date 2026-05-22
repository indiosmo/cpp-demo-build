#include "morfix/session.hpp"

#include "lab/variant.hpp"

namespace morfix {

types::exec_id lifecycle_state::next_exec_id()
{
  return types::exec_id{next_exec_id_value_++};
}

void lifecycle_state::remember(const mor::request& request)
{
  lab::match(
    request,
    [this, &request](const mor::new_order_single& order) { requests_by_cl_ord_id_[order.cl_ord_id] = request; },
    [this, &request](const mor::replace_request& replace) { requests_by_cl_ord_id_[replace.cl_ord_id] = request; },
    [this, &request](const mor::cancel_request& cancel) { requests_by_cl_ord_id_[cancel.cl_ord_id] = request; },
    [](const mor::flush_request&) {});
}

std::optional<mor::request> lifecycle_state::find(types::cl_ord_id cl_ord_id) const
{
  if (auto it = requests_by_cl_ord_id_.find(cl_ord_id); it != requests_by_cl_ord_id_.end()) {
    return it->second;
  }
  return std::nullopt;
}

} // namespace morfix
