#include "server/application.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "morfix_quickfix/codecs.hpp"
#include "order_entry/runtime/session_config.hpp"
#include "quickfix_fix/session.hpp"

#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/json.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace {

struct thread_config
{
  lab::event_loop_config input{
    .thread_name = "lab-input",
    .idle_strategy = lab::busy_spin_idle{},
  };

  lab::event_loop_config processing{
    .thread_name = "lab-engine",
    .idle_strategy = lab::busy_spin_idle{},
  };

  lab::event_loop_config output{
    .thread_name = "lab-output",
    .idle_strategy = lab::busy_spin_idle{},
  };
};

LAB_AUTO_JSON(thread_config, input, processing, output)

struct fix_config
{
  LAB_DEFAULTED_FIELD(bool, enabled, false);
  std::optional<quickfix_fix::session_pair_config> session;
  std::optional<morfix_quickfix::b3_codec_config> b3;
};

LAB_AUTO_JSON(fix_config, enabled, session, b3)

struct app_config
{
  order_entry::runtime::session_config order_entry;
  matching_engine::runtime::engine_config matching_engine;
  market_data::runtime::publisher_config market_data;
  lab::logger_config logger{lab::console_logger_config{}};
  thread_config threads;
  std::optional<fix_config> fix;
};

LAB_AUTO_JSON(app_config, order_entry, matching_engine, market_data, logger, threads, fix)

server::config make_runtime_config(const app_config& config)
{
  return server::config{
    .order_entry = config.order_entry,
    .matching_engine = config.matching_engine,
    .market_data = config.market_data,
    .logger = config.logger,
    .input_thread = config.threads.input,
    .processing_thread = config.threads.processing,
    .output_thread = config.threads.output,
  };
}

void print_usage(std::ostream& stream)
{
  stream << "usage: server CONFIG_FILE\n";
}

lab::result<std::string> parse_config_path(int argc, char** argv)
{
  if (argc == 2) {
    const std::string argument{argv[1]};
    if (argument == "--help" || argument == "-h") {
      print_usage(std::cout);
      return {};
    }
    return argument;
  }

  return lab::make_leaf_error(lab::error_code::configuration_error, "expected one config file path");
}

std::atomic<bool> g_stop_requested = false;

void signal_handler(int /* signal */)
{
  g_stop_requested.store(true, std::memory_order_release);
}

} // namespace

int main(int argc, char** argv)
{
  const auto result = boost::leaf::try_handle_all(
    [&]() -> lab::result<int> {
      BOOST_LEAF_ASSIGN(const auto config_path, parse_config_path(argc, argv));
      if (config_path.empty()) {
        return EXIT_SUCCESS;
      }

      BOOST_LEAF_ASSIGN(const auto config, lab::json::read_from_file<app_config>(config_path, true));
      server::application app{make_runtime_config(config)};

      std::signal(SIGINT, signal_handler);
      std::signal(SIGTERM, signal_handler);

      LAB_LEAF_CHECK(app.start());

      // TODO: replace the signal poll with a lifecycle notification primitive.
      while (!g_stop_requested.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
      }

      app.stop();
      return EXIT_SUCCESS;
    },
    LAB_RESULT_CATCH_ALL(print_usage(std::cerr); return EXIT_FAILURE;));

  return result;
}
