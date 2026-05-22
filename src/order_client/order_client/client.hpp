#ifndef ORDER_CLIENT_CLIENT_HPP
#define ORDER_CLIENT_CLIENT_HPP

#include "lab/network/types.hpp"
#include "lab/result.hpp"
#include "order_client/json_encoder.hpp"
#include "order_client/udp_sender.hpp"
#include "order_entry/messages.hpp"

namespace order_client {

struct client_config
{
  udp_sender_config sender{
    .endpoint =
      lab::network::types::endpoint_config{
        .address = "127.0.0.1",
        .port = 1234,
      },
    .max_datagram_size = 65535,
  };
};

BOOST_DESCRIBE_STRUCT(client_config, (), (sender))

inline void to_json(nlohmann::json& json_object, const client_config& config)
{
  lab::json::write_described_object(json_object, config);
}

inline void from_json(const nlohmann::json& json_object, client_config& config)
{
  if (json_object.contains("sender")) {
    lab::json::read_field(json_object, "sender", config.sender);
    return;
  }

  if (json_object.contains("endpoint")) {
    lab::json::read_field(json_object, "endpoint", config.sender.endpoint);
  }
  if (json_object.contains("max_datagram_size")) {
    lab::json::read_field(json_object, "max_datagram_size", config.sender.max_datagram_size);
  }
}

using config = client_config;

class client
{
public:
  explicit client(client_config configuration);

  lab::result<void> connect();
  lab::result<void> send(const order_entry::new_order_single& order);
  lab::result<void> send(const order_entry::replace_order& replace);
  lab::result<void> send(const order_entry::cancel_order& cancel);
  lab::result<void> send(const order_entry::flush& flush_request);
  lab::result<void> send(const order_entry::request& request);

private:
  json_encoder encoder_;
  udp_sender sender_;
};

} // namespace order_client

#endif /* ORDER_CLIENT_CLIENT_HPP */
