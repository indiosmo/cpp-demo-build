#ifndef ORDER_CLIENT_CLIENT_HPP
#define ORDER_CLIENT_CLIENT_HPP

#include "order_client/csv_encoder.hpp"
#include "order_client/udp_sender.hpp"
#include "order_routing/messages.hpp"

#include "lab/network/types.hpp"
#include "lab/result.hpp"

namespace order_client {

struct config
{
  lab::network::types::endpoint_config endpoint{
    .address = "127.0.0.1",
    .port = 1234,
  };
};

class client
{
public:
  explicit client(config configuration);

  lab::result<void> connect();
  lab::result<void> send(const order_routing::new_order& order);
  lab::result<void> send(const order_routing::cancel_order& cancel);
  lab::result<void> send(const order_routing::flush& flush_request);
  lab::result<void> send(const order_routing::request& request);

private:
  csv_encoder encoder_;
  udp_sender sender_;
};

} // namespace order_client

#endif /* ORDER_CLIENT_CLIENT_HPP */
