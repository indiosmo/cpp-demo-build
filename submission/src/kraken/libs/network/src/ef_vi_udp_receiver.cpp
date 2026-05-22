#include "kraken/network/ef_vi_udp_receiver.hpp"

#include "kraken/error.hpp"
#include "kraken/error_code.hpp"
#include "kraken/result.hpp"

#include <cstddef>
#include <utility>

namespace kraken::network {

ef_vi_udp_receiver::ef_vi_udp_receiver(types::endpoint_config config)
  : config_{std::move(config)}
{
}

kraken::result<void> ef_vi_udp_receiver::open()
{
  // Fails at open() so a misconfigured backend choice surfaces at start-up.
  return kraken::make_leaf_error(
    kraken::error_code::not_implemented, "ef_vi backend is a stub; select asio_udp_receiver instead");
}

std::size_t ef_vi_udp_receiver::poll()
{
  return 0;
}

void ef_vi_udp_receiver::close() {}

} // namespace kraken::network
