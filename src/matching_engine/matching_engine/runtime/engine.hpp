#ifndef MATCHING_ENGINE_RUNTIME_ENGINE_HPP
#define MATCHING_ENGINE_RUNTIME_ENGINE_HPP

#include "market_data/messages.hpp"
#include "matching_engine/engine.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_entry/messages.hpp"

#include "lab/inplace_function.hpp"
#include "lab/result.hpp"

#include <optional>

/*
 * Runtime composer for the matching engine. Owns the domain engine
 * built from the runtime config and re-exports its send() / event
 * surface for the wiring shell.
 */

namespace matching_engine::runtime {

class engine
{
public:
  /*
   * Raised on the processing-loop thread for each engine event, in the
   * order docs/engine-specs.md specifies.
   */
  lab::inplace_function<void(const market_data::message&)> on_market_data;
  lab::inplace_function<void(const order_entry::event&)> on_order_entry;

  engine() = default;
  engine(const engine&) = delete;
  engine(engine&&) = delete;
  engine& operator=(const engine&) = delete;
  engine& operator=(engine&&) = delete;
  ~engine();

  /* Precondition: callbacks are wired before any send() call. */
  lab::result<void> setup(engine_config config);

  /* Precondition: called on the engine's owning thread. */
  void send(const order_entry::request& req);

private:
  std::optional<matching_engine::engine> impl_;
};

} // namespace matching_engine::runtime

#endif /* MATCHING_ENGINE_RUNTIME_ENGINE_HPP */
