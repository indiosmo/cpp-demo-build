#include "kraken/network/asio_udp_receiver.hpp"

#include "boost/asio/buffer.hpp"
#include "boost/asio/ip/address.hpp"
#include "boost/asio/ip/udp.hpp"
#include "boost/asio/socket_base.hpp"

#include "kraken/error.hpp"
#include "kraken/error_code.hpp"
#include "kraken/result.hpp"

#include <string>
#include <system_error>
#include <tuple>
#include <utility>

namespace kraken::network {

namespace {

kraken::result<boost::asio::ip::address> parse_address(const std::string& text)
{
  boost::system::error_code error;
  auto address = boost::asio::ip::make_address(text, error);
  if (error) {
    return kraken::make_leaf_error(kraken::error_code::configuration_error, error.message());
  }
  return address;
}

kraken::result<void> open_socket(boost::asio::ip::udp::socket& socket, const boost::asio::ip::udp::endpoint& endpoint)
{
  boost::system::error_code error;
  if (const auto open_error = socket.open(endpoint.protocol(), error)) {
    return kraken::make_leaf_error(kraken::error_code::generic_error, open_error.message());
  }
  return {};
}

kraken::result<void> enable_reuse_address(boost::asio::ip::udp::socket& socket)
{
  boost::system::error_code error;
  if (const auto option_error = socket.set_option(boost::asio::socket_base::reuse_address{true}, error)) {
    return kraken::make_leaf_error(kraken::error_code::generic_error, option_error.message());
  }
  return {};
}

kraken::result<void> bind_socket(boost::asio::ip::udp::socket& socket, const boost::asio::ip::udp::endpoint& endpoint)
{
  boost::system::error_code error;
  if (const auto bind_error = socket.bind(endpoint, error)) {
    return kraken::make_leaf_error(kraken::error_code::generic_error, bind_error.message());
  }
  return {};
}

} // namespace

asio_udp_receiver::asio_udp_receiver(boost::asio::io_context& io_context, types::endpoint_config config)
  : config_{std::move(config)}
  , socket_{io_context}
{
}

kraken::result<void> asio_udp_receiver::start()
{
  BOOST_LEAF_ASSIGN(const auto address, parse_address(config_.address));
  const boost::asio::ip::udp::endpoint endpoint{address, config_.port};

  KRAKEN_LEAF_CHECK(open_socket(socket_, endpoint));
  KRAKEN_LEAF_CHECK(enable_reuse_address(socket_));
  KRAKEN_LEAF_CHECK(bind_socket(socket_, endpoint));

  post_receive();
  return {};
}

void asio_udp_receiver::stop()
{
  boost::system::error_code ignored;
  std::ignore = socket_.cancel(ignored);
}

void asio_udp_receiver::post_receive()
{
  socket_.async_receive_from(
    boost::asio::buffer(receive_buffer_.data(), receive_buffer_.size()),
    sender_endpoint_,
    [this](const boost::system::error_code& error, std::size_t bytes_received) {
      if (error) {
        // operation_aborted is the expected outcome of stop(); any other code
        // signals socket failure, so the receive loop ends here.
        return;
      }

      if (on_datagram) {
        on_datagram(types::datagram_view{receive_buffer_.data(), bytes_received});
      }

      post_receive();
    });
}

} // namespace kraken::network
