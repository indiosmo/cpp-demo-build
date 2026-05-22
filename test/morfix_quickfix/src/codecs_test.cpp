#include "catch2/catch_test_macros.hpp"
#include "lab/error.hpp"
#include "morfix_quickfix/codecs.hpp"
#include "morfix_quickfix/error_code.hpp"

#include <boost/leaf/handle_errors.hpp>
#include <optional>
#include <system_error>
#include <utility>

namespace {

template <typename Fn>
std::error_code capture_error_code(Fn&& fn)
{
  return boost::leaf::try_handle_all(
           [&]() -> lab::result<std::optional<std::error_code>> {
             auto result = std::forward<Fn>(fn)();
             if (result) {
               return std::nullopt;
             }

             return result.error();
           },
           [](const lab::error& err) { return std::optional{err.error_code()}; },
           [](const std::error_code& ec) { return std::optional{ec}; },
           [] { return std::optional<std::error_code>{}; })
    .value_or(std::error_code{});
}

} // namespace

TEST_CASE("morfix_quickfix b3 initiator - request encoding returns typed scaffold failure", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::initiator codec;
  const morfix::request request = morfix::order_cancel_request{
    .account = morfix::types::client_id{7},
    .cl_ord_id = morfix::types::cl_ord_id{99},
    .orig_cl_ord_id = morfix::types::orig_cl_ord_id{42},
  };

  const auto ec = capture_error_code([&] { return codec.encode(request); });
  CHECK(ec == morfix_quickfix::error_code::not_implemented);
}

TEST_CASE("morfix_quickfix b3 acceptor - unknown FIX message returns typed failure", "[morfix_quickfix][codec]")
{
  const morfix_quickfix::codecs::b3::acceptor codec;
  const quickfix_fix::message message{"D"};

  const auto ec = capture_error_code([&] { return codec.decode(message); });
  CHECK(ec == morfix_quickfix::error_code::unsupported_message);
}

