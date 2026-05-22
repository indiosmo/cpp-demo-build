#include "order_client/client.hpp"

#include "lab/result.hpp"
#include "lab/variant.hpp"

#include <utility>

namespace order_client {

client::client(client_config configuration)
  : sender_{std::move(configuration.sender)}
{
}

lab::result<void> client::connect()
{
  return sender_.connect();
}

lab::result<void> client::send(const order_entry::new_order_single& order)
{
  const auto payload = encoder_.encode(order);
  LAB_LEAF_CHECK(sender_.send(payload));
  return {};
}

lab::result<void> client::send(const order_entry::replace_order& replace)
{
  const auto payload = encoder_.encode(replace);
  LAB_LEAF_CHECK(sender_.send(payload));
  return {};
}

lab::result<void> client::send(const order_entry::cancel_order& cancel)
{
  const auto payload = encoder_.encode(cancel);
  LAB_LEAF_CHECK(sender_.send(payload));
  return {};
}

lab::result<void> client::send(const order_entry::flush& flush_request)
{
  const auto payload = encoder_.encode(flush_request);
  LAB_LEAF_CHECK(sender_.send(payload));
  return {};
}

lab::result<void> client::send(const order_entry::request& request)
{
  return lab::match(request, [this](const auto& command) { return send(command); });
}

} // namespace order_client
