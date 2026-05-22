#ifndef MARKET_DATA_RUNTIME_PUBLISHER_HPP
#define MARKET_DATA_RUNTIME_PUBLISHER_HPP

#include "market_data/encoder.hpp"
#include "market_data/messages.hpp"
#include "market_data/publisher.hpp"
#include "market_data/runtime/publisher_config.hpp"
#include "market_data/sink.hpp"

#include "lab/result.hpp"

#include <memory>
#include <optional>

/*
 * Runtime composer for the output pipeline's terminal stage. Owns the
 * encoder and sink chosen by publisher_config and wires them into a
 * market_data::publisher.
 */

namespace market_data::runtime {

class publisher
{
public:
  publisher() = default;
  publisher(const publisher&) = delete;
  publisher(publisher&&) = delete;
  publisher& operator=(const publisher&) = delete;
  publisher& operator=(publisher&&) = delete;
  ~publisher();

  lab::result<void> setup(const publisher_config& config);

  // precondition: invoked from the output loop's thread; the sink is
  // single-threaded by construction.
  void send(const market_data::message& msg);

private:
  void setup_encoder(const csv_encoder_config& config);
  void setup_sink(const spdlog_sink_config& config);

  // Declared before publisher_ so the inner publisher (which holds references
  // to both) tears down first.
  std::unique_ptr<encoder> encoder_;
  std::unique_ptr<sink> sink_;

  std::optional<market_data::publisher> publisher_;
};

} // namespace market_data::runtime

#endif /* MARKET_DATA_RUNTIME_PUBLISHER_HPP */
