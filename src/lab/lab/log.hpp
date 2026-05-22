#ifndef LAB_LOG_HPP
#define LAB_LOG_HPP

#include "lab/fmt.hpp"

#include <cstdio>
#include <exception>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

/*
 * Diagnostic logging facade. Market-data records own stdout, so diagnostics
 * default to stderr and can be switched to a file or a null backend at
 * startup. DEPENDENCIES.md records the logging dependency boundary.
 */

namespace lab {

enum class log_level
{
  trace,
  debug,
  info,
  warn,
  error,
  critical,
};

[[nodiscard]] inline constexpr std::string_view to_string(log_level level) noexcept
{
  switch (level) {
    case log_level::trace:
      return "trace";
    case log_level::debug:
      return "debug";
    case log_level::info:
      return "info";
    case log_level::warn:
      return "warn";
    case log_level::error:
      return "error";
    case log_level::critical:
      return "critical";
  }

  std::terminate();
}

struct null_logger
{
  void write(std::string_view) const noexcept {}
};

struct console_logger
{
  void write(std::string_view line) const
  {
    // stdout carries market-data records; diagnostics stay on stderr.
    fmt::print(stderr, "{}\n", line);
  }
};

class file_logger
{
public:
  explicit file_logger(std::string path)
    : stream_{std::move(path), std::ios::app}
  {
  }

  void write(std::string_view line)
  {
    stream_ << line << '\n';
    stream_.flush();
  }

private:
  std::ofstream stream_;
};

using logger_variant = std::variant<console_logger, null_logger, file_logger>;

inline logger_variant logger{console_logger{}};
inline std::mutex logger_mutex;

/*
 * Startup-time selection of which logger backs the global `logger`. Plain
 * data so the choice can be built without holding stream state.
 */
struct console_logger_config
{
};

struct null_logger_config
{
};

struct file_logger_config
{
  std::string path;
};

using logger_config = std::variant<console_logger_config, null_logger_config, file_logger_config>;

inline void install_logger(const logger_config& config)
{
  const std::lock_guard lock{logger_mutex};
  std::visit(
    [](const auto& selection) {
      using selection_t = std::decay_t<decltype(selection)>;
      if constexpr (std::is_same_v<selection_t, console_logger_config>) {
        logger.emplace<console_logger>();
      } else if constexpr (std::is_same_v<selection_t, null_logger_config>) {
        logger.emplace<null_logger>();
      } else if constexpr (std::is_same_v<selection_t, file_logger_config>) {
        logger.emplace<file_logger>(selection.path);
      }
    },
    config);
}

template <typename... Args>
[[nodiscard]] std::string
format_log_line(log_level level, std::source_location location, fmt::format_string<Args...> format, Args&&... args)
{
  return fmt::format(
    "[{}] {}:{} | {}", to_string(level), location.file_name(), location.line(), fmt::format(format, std::forward<Args>(args)...));
}

template <log_level Level, typename... Args>
void log(std::source_location location, fmt::format_string<Args...> format, Args&&... args)
{
  const auto line = format_log_line(Level, location, format, std::forward<Args>(args)...);
  const std::lock_guard lock{logger_mutex};
  std::visit([&](auto& impl) { impl.write(line); }, logger);
}

} // namespace lab

#define LAB_LOG_TRACE(...)    ::lab::log<::lab::log_level::trace>(std::source_location::current(), __VA_ARGS__)
#define LAB_LOG_DEBUG(...)    ::lab::log<::lab::log_level::debug>(std::source_location::current(), __VA_ARGS__)
#define LAB_LOG_INFO(...)     ::lab::log<::lab::log_level::info>(std::source_location::current(), __VA_ARGS__)
#define LAB_LOG_WARN(...)     ::lab::log<::lab::log_level::warn>(std::source_location::current(), __VA_ARGS__)
#define LAB_LOG_ERROR(...)    ::lab::log<::lab::log_level::error>(std::source_location::current(), __VA_ARGS__)
#define LAB_LOG_CRITICAL(...) ::lab::log<::lab::log_level::critical>(std::source_location::current(), __VA_ARGS__)

#endif /* LAB_LOG_HPP */
