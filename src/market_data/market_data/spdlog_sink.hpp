#ifndef MARKET_DATA_SPDLOG_SINK_HPP
#define MARKET_DATA_SPDLOG_SINK_HPP

#include "market_data/sink.hpp"

#include <memory>
#include <string_view>

/*
 * Sink backed by an spdlog stdout logger. The runtime pins this sink to a
 * single thread, so the single-threaded logger variant is used to skip the
 * locking that a multi-threaded one would do on every write.
 */

namespace spdlog {
class logger;
}

namespace market_data {

class spdlog_sink final : public sink
{
public:
  spdlog_sink();
  ~spdlog_sink() override;

  void write(std::string_view record) override;

private:
  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace market_data

#endif /* MARKET_DATA_SPDLOG_SINK_HPP */
