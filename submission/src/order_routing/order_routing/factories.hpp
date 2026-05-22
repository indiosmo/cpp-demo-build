#ifndef ORDER_ROUTING_FACTORIES_HPP
#define ORDER_ROUTING_FACTORIES_HPP

#include "order_routing/errors.hpp"
#include "order_routing/messages.hpp"

#include <string>
#include <string_view>

/*
 * make_rejection overloads, one per structured decoder error. session::send
 * dispatches on the concrete error type via kraken::match_error.
 */

namespace order_routing {

inline rejection make_rejection(const errors::invalid_field& err, std::string_view raw_payload)
{
  return rejection{
    .raw_payload = std::string{raw_payload},
    .reason = err.what(),
  };
}

inline rejection make_rejection(const errors::missing_field& err, std::string_view raw_payload)
{
  return rejection{
    .raw_payload = std::string{raw_payload},
    .reason = err.what(),
  };
}

inline rejection make_rejection(const errors::parser_error& err, std::string_view raw_payload)
{
  return rejection{
    .raw_payload = std::string{raw_payload},
    .reason = err.what(),
  };
}

} // namespace order_routing

#endif /* ORDER_ROUTING_FACTORIES_HPP */
