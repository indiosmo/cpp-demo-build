#include "catch2/catch_test_macros.hpp"
#include "morfix/session.hpp"

#include <variant>

TEST_CASE("morfix lifecycle_state - allocates exec ids monotonically", "[morfix][unit]")
{
  morfix::lifecycle_state state;

  CHECK(state.next_exec_id() == morfix::types::exec_id{1});
  CHECK(state.next_exec_id() == morfix::types::exec_id{2});
}

TEST_CASE("morfix lifecycle_state - stores request correlation by ClOrdID", "[morfix][unit]")
{
  morfix::lifecycle_state state;
  const mor::request request = mor::cancel_request{
    .client_id = mor::types::client_id{7},
    .cl_ord_id = mor::types::cl_ord_id{99},
    .orig_cl_ord_id = mor::types::orig_cl_ord_id{42},
  };

  state.remember(request);

  const auto found = state.find(morfix::types::cl_ord_id{99});
  REQUIRE(found.has_value());
  REQUIRE(std::holds_alternative<mor::cancel_request>(*found));
  CHECK(std::get<mor::cancel_request>(*found).orig_cl_ord_id == mor::types::orig_cl_ord_id{42});
  CHECK_FALSE(state.find(morfix::types::cl_ord_id{100}).has_value());
}

