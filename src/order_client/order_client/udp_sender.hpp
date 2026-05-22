#ifndef ORDER_CLIENT_UDP_SENDER_HPP
#define ORDER_CLIENT_UDP_SENDER_HPP

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/udp.hpp"

#include "lab/json.hpp"
#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include <cstddef>
#include <string_view>

namespace order_client {

struct udp_sender_config
{
  lab::network::types::endpoint_config endpoint;
  std::size_t max_datagram_size;
};

LAB_AUTO_JSON(udp_sender_config, endpoint, max_datagram_size)

class udp_sender
{
public:
  explicit udp_sender(udp_sender_config config);

  lab::result<void> connect();
  lab::result<void> send(std::string_view payload);

private:
  udp_sender_config config_;
  boost::asio::io_context io_context_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint endpoint_;
  bool connected_ = false;
};

} // namespace order_client

#endif /* ORDER_CLIENT_UDP_SENDER_HPP */
