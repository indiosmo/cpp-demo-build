#include "order_client/client.hpp"

#include "lab/result.hpp"
#include "lab/variant.hpp"

#include <utility>

namespace order_client {

client::client(config configuration)
  : sender_{std::move(configuration.endpoint)}
{
}

lab::result<void> client::connect()
{
  return sender_.connect();
}

lab::result<void> client::send(const order_routing::new_order& order)
{
  const auto payload = encoder_.encode(order);
  LAB_LEAF_CHECK(sender_.send(payload));
  return {};
}

lab::result<void> client::send(const order_routing::cancel_order& cancel)
{
  const auto payload = encoder_.encode(cancel);
  LAB_LEAF_CHECK(sender_.send(payload));
  return {};
}

lab::result<void> client::send(const order_routing::flush& flush_request)
{
  const auto payload = encoder_.encode(flush_request);
  LAB_LEAF_CHECK(sender_.send(payload));
  return {};
}

lab::result<void> client::send(const order_routing::request& request)
{
  return lab::match(request, [this](const auto& command) { return send(command); });
}

} // namespace order_client
