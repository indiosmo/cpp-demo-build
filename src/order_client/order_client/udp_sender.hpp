#ifndef ORDER_CLIENT_UDP_SENDER_HPP
#define ORDER_CLIENT_UDP_SENDER_HPP

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/udp.hpp"

#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include <string_view>

namespace order_client {

class udp_sender
{
public:
  explicit udp_sender(lab::network::types::endpoint_config endpoint);

  lab::result<void> connect();
  lab::result<void> send(std::string_view payload);

private:
  lab::network::types::endpoint_config endpoint_config_;
  boost::asio::io_context io_context_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint endpoint_;
  bool connected_ = false;
};

} // namespace order_client

#endif /* ORDER_CLIENT_UDP_SENDER_HPP */
