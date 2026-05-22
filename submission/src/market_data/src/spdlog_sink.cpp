#include "market_data/spdlog_sink.hpp"

#include <memory>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <utility>

namespace market_data {

spdlog_sink::spdlog_sink()
{
  auto stdout_sink = std::make_shared<spdlog::sinks::stdout_sink_st>();
  // Pass the encoded record through verbatim, with only a trailing newline
  // appended; no timestamp, level, or other log decoration.
  stdout_sink->set_pattern("%v");

  logger_ = std::make_shared<spdlog::logger>("market_data", std::move(stdout_sink));
  logger_->set_level(spdlog::level::info);
  // Flush per record: buffering would let a crash swallow already-emitted data.
  logger_->flush_on(spdlog::level::info);
}

spdlog_sink::~spdlog_sink() = default;

void spdlog_sink::write(std::string_view record)
{
  // Pass the record as a format argument rather than as the format string
  // so any '{' in the payload is treated as data, not as a placeholder.
  logger_->info("{}", record);
}

} // namespace market_data
