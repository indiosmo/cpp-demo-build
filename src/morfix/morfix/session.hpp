#ifndef MORFIX_SESSION_HPP
#define MORFIX_SESSION_HPP

#include "mor/messages.hpp"
#include "morfix/messages.hpp"

#include <optional>
#include <unordered_map>

namespace morfix {

class lifecycle_state
{
public:
  [[nodiscard]] types::exec_id next_exec_id();

  void remember(const mor::request& request);

  [[nodiscard]] std::optional<mor::request> find(types::cl_ord_id cl_ord_id) const;

private:
  std::uint64_t next_exec_id_value_{1};
  std::unordered_map<types::cl_ord_id, mor::request> requests_by_cl_ord_id_;
};

} // namespace morfix

#endif /* MORFIX_SESSION_HPP */
