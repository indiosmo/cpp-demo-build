#ifndef MATCHING_ENGINE_RUNTIME_ENGINE_HPP
#define MATCHING_ENGINE_RUNTIME_ENGINE_HPP

#include "market_data/messages.hpp"
#include "matching_engine/engine.hpp"
#include "matching_engine/runtime/engine_config.hpp"
#include "order_routing/messages.hpp"

#include "kraken/inplace_function.hpp"
#include "kraken/result.hpp"

#include <optional>

/*
 * Runtime composer for the matching engine. Owns the domain engine
 * built from the runtime config and re-exports its send() / on_event
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
  kraken::inplace_function<void(const market_data::message&)> on_event;

  engine() = default;
  engine(const engine&) = delete;
  engine(engine&&) = delete;
  engine& operator=(const engine&) = delete;
  engine& operator=(engine&&) = delete;
  ~engine();

  /* Precondition: on_event is wired before any send() call. */
  kraken::result<void> setup(engine_config config);

  /* Precondition: called on the engine's owning thread. */
  void send(const order_routing::request& req);

private:
  std::optional<matching_engine::engine> impl_;
};

} // namespace matching_engine::runtime

#endif /* MATCHING_ENGINE_RUNTIME_ENGINE_HPP */
