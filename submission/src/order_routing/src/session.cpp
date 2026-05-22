#include "order_routing/session.hpp"

#include "boost/leaf/handle_errors.hpp"
#include "order_routing/decoder.hpp"
#include "order_routing/errors.hpp"
#include "order_routing/factories.hpp"
#include "order_routing/messages.hpp"

#include "kraken/algorithm.hpp"
#include "kraken/error.hpp"
#include "kraken/expected.hpp"
#include "kraken/result.hpp"

#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace order_routing {

session::session(const decoder& packet_decoder)
  : decoder_{&packet_decoder}
{
}

void session::send(std::string_view packet)
{
  if (kraken::trim(packet).empty()) {
    return;
  }

  // Funnel every decoder failure path -- structured errors and stray
  // exceptions alike -- into a rejection so the dispatch below is a
  // single if/else over expected.
  using outcome = kraken::expected<request, rejection>;

  auto result = boost::leaf::try_handle_all(
    [&]() -> kraken::result<outcome> {
      BOOST_LEAF_ASSIGN(auto decoded, decoder_->decode(packet));
      return outcome{std::move(decoded)};
    },
    [&](kraken::match_error<errors::invalid_field> matched) -> outcome {
      return kraken::unexpected(make_rejection(matched.value(), packet));
    },
    [&](kraken::match_error<errors::missing_field> matched) -> outcome {
      return kraken::unexpected(make_rejection(matched.value(), packet));
    },
    [&](kraken::match_error<errors::parser_error> matched) -> outcome {
      return kraken::unexpected(make_rejection(matched.value(), packet));
    },
    [&](const kraken::error& err) -> outcome {
      return kraken::unexpected(
        rejection{
          .raw_payload = std::string{packet},
          .reason = err.what(),
        });
    },
    [&](const std::exception& ex) -> outcome {
      return kraken::unexpected(
        rejection{
          .raw_payload = std::string{packet},
          .reason = ex.what(),
        });
    },
    [&]() -> outcome {
      return kraken::unexpected(
        rejection{
          .raw_payload = std::string{packet},
          .reason = "unknown decoder failure",
        });
    });

  // Notify subscribers outside the error handler so an exception thrown
  // by a subscriber propagates to the caller instead of being caught and
  // turned into a rejection.
  if (result) {
    on_request(*result);
  } else {
    on_rejected(result.error());
  }
}

} // namespace order_routing
