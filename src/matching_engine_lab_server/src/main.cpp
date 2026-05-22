#include "matching_engine_lab_server/application.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_routing/runtime/session_config.hpp"
#include "order_routing/types.hpp"

#include "lab/network/types.hpp"
#include "lab/result.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <thread>

namespace {

// The sample UDP protocol uses port 1234 by default.
constexpr std::uint16_t default_udp_port = 1234;
constexpr const char* default_listen_address = "0.0.0.0";

// Pre-allocated symbol set matches the provided test corpus; other symbols
// are rejected at the engine boundary. A real deployment would source this
// list from an exchange-membership feed at startup.
matching_engine::runtime::engine_config make_engine_config()
{
  return matching_engine::runtime::engine_config{
    .valid_symbols{
      order_routing::types::symbol{"IBM"},
      order_routing::types::symbol{"AAPL"},
      order_routing::types::symbol{"VAL"},
    },
  };
}

std::atomic<bool> g_stop_requested = false;

void signal_handler(int /* signal */)
{
  g_stop_requested.store(true, std::memory_order_release);
}

} // namespace

int main()
{
  matching_engine_lab_server::application app{matching_engine_lab_server::config{
    .order_routing{
      .receiver{order_routing::runtime::asio_udp_receiver_config{
        .endpoint{
          .address = default_listen_address,
          .port = default_udp_port,
        },
      }},
    },
    .matching_engine = make_engine_config(),
    .market_data{},
  }};

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  LAB_EXIT_ON_ERROR(app.start());

  // IMPROVEMENT: replace the poll with a condition variable signalled when
  // every loop has stopped and drained.
  while (!g_stop_requested.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  app.stop();
  return EXIT_SUCCESS;
}
