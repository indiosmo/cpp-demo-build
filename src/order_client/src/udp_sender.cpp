#include "order_client/udp_sender.hpp"

#include "boost/asio/buffer.hpp"
#include "boost/asio/ip/address.hpp"

#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/result.hpp"

#include <string>
#include <system_error>
#include <utility>

namespace order_client {
namespace {

lab::result<boost::asio::ip::address> parse_address(const std::string& text)
{
  boost::system::error_code error;
  auto address = boost::asio::ip::make_address(text, error);
  if (error) {
    return lab::make_leaf_error(lab::error_code::configuration_error, error.message());
  }
  return address;
}

} // namespace

udp_sender::udp_sender(udp_sender_config config)
  : config_{std::move(config)}
  , socket_{io_context_}
{
}

lab::result<void> udp_sender::connect()
{
  if (connected_) {
    return {};
  }

  BOOST_LEAF_ASSIGN(const auto address, parse_address(config_.endpoint.address));
  endpoint_ = boost::asio::ip::udp::endpoint{address, config_.endpoint.port};

  boost::system::error_code error;
  socket_.open(endpoint_.protocol(), error);
  if (error) {
    return lab::make_leaf_error(lab::error_code::generic_error, error.message());
  }

  connected_ = true;
  return {};
}

lab::result<void> udp_sender::send(std::string_view payload)
{
  if (payload.size() > config_.max_datagram_size) {
    return lab::make_leaf_error(lab::error_code::invalid_argument, "datagram exceeds configured maximum size");
  }

  LAB_LEAF_CHECK(connect());

  boost::system::error_code error;
  socket_.send_to(boost::asio::buffer(payload.data(), payload.size()), endpoint_, 0, error);
  if (error) {
    return lab::make_leaf_error(lab::error_code::generic_error, error.message());
  }

  return {};
}

} // namespace order_client
