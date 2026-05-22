#ifndef MATCHING_ENGINE_LAB_SERVER_APPLICATION_HPP
#define MATCHING_ENGINE_LAB_SERVER_APPLICATION_HPP

#include "boost/asio/io_context.hpp"
#include "market_data/runtime/publisher.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "matching_engine/runtime/engine.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_routing/runtime/session.hpp"
#include "order_routing/runtime/session_config.hpp"

#include "lab/event_loop.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"

#include <optional>

/*
 * Wiring shell for the three-thread pipeline (input loop -> processing loop
 * -> output loop). Owns the loops and the three runtime composers; backend
 * selection happens through config_. See docs/runtime-startup.md for the
 * startup sequence.
 */

namespace matching_engine_lab_server {

struct config
{
  order_routing::runtime::session_config order_routing;
  matching_engine::runtime::engine_config matching_engine;
  market_data::runtime::publisher_config market_data;
  lab::logger_config logger{lab::console_logger_config{}};

  /*
   * busy_spin_idle by default; override per-loop with lab::timed_wait_idle
   * to give back the idle core. See docs/event-loop.md.
   */
  lab::event_loop_config input_thread{
    .thread_name = "lab-input",
    .idle_strategy = lab::busy_spin_idle{},
  };

  lab::event_loop_config processing_thread{
    .thread_name = "lab-engine",
    .idle_strategy = lab::busy_spin_idle{},
  };

  lab::event_loop_config output_thread{
    .thread_name = "lab-output",
    .idle_strategy = lab::busy_spin_idle{},
  };
};

class application
{
public:
  explicit application(config configuration);
  application(const application&) = delete;
  application(application&&) = delete;
  application& operator=(const application&) = delete;
  application& operator=(application&&) = delete;
  ~application();

  lab::result<void> start();

  /*
   * Joins the three loop threads. stop() must be invoked from another thread
   * (typically a signal handler in main).
   */
  void run();

  /* Tears the pipeline down inbound-to-outbound; idempotent. */
  void stop();

private:
  void configure_logger();
  void wire_pipeline();

  config config_;
  /*
   * Only the asio receiver backend uses this. Declared before order_routing_
   * so the receiver tears down before the context it borrows from.
   */
  std::optional<boost::asio::io_context> io_context_;
  lab::event_loop input_loop_;
  lab::event_loop processing_loop_;
  lab::event_loop output_loop_;
  order_routing::runtime::session order_routing_;
  matching_engine::runtime::engine engine_;
  market_data::runtime::publisher publisher_;
  bool started_ = false;
};

} // namespace matching_engine_lab_server

#endif /* MATCHING_ENGINE_LAB_SERVER_APPLICATION_HPP */
