#ifndef LAB_EVENT_LOOP_HPP
#define LAB_EVENT_LOOP_HPP

#include "lab/assert.hpp"
#include "lab/concurrent_queue.hpp"
#include "lab/error.hpp"
#include "lab/error_code.hpp"
#include "lab/inplace_function.hpp"
#include "lab/log.hpp"
#include "lab/result.hpp"
#include "lab/variant.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <pthread.h>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace lab {

/*
 * Idle strategy picked once at start; selects which inner run loop the
 * consumer thread executes.
 */
struct timed_wait_idle
{
  // Maximum time parked on the task queue before re-checking running_ and pollers.
  std::chrono::microseconds duration{1000};
};

struct busy_spin_idle
{
};

using event_loop_idle_strategy = std::variant<timed_wait_idle, busy_spin_idle>;

struct event_loop_config
{
  std::string thread_name;
  std::size_t queue_capacity = 1024;
  // Bounded wait is the default; busy_spin_idle is the HFT override.
  event_loop_idle_strategy idle_strategy{timed_wait_idle{}};
};

/*
 * Thread-affine task loop. A single consumer thread runs a queue of posted
 * tasks plus optional pollers so edge adapters can share a thread without
 * owning one.
 */
class event_loop
{
public:
  using task_t = lab::inplace_function<void(), 2048>;
  using poller_t = lab::inplace_function<bool(), 128>;

  explicit event_loop(event_loop_config config)
    : config_{std::move(config)}
    , task_queue_{config_.queue_capacity}
  {
  }

  event_loop(const event_loop&) = delete;
  event_loop(event_loop&&) = delete;
  event_loop& operator=(const event_loop&) = delete;
  event_loop& operator=(event_loop&&) = delete;

  ~event_loop()
  {
    stop();
    join();
  }

  void add_poller(poller_t poller)
  {
    pollers_.push_back(std::move(poller));
  }

  lab::result<void> start()
  {
    if (running_.load(std::memory_order_acquire)) {
      return lab::make_leaf_error(lab::error_code::already_in_progress, "event loop already running");
    }

    running_.store(true, std::memory_order_release);

    try {
      thread_ = std::jthread{[this] { run(); }};
    } catch (const std::exception& ex) {
      running_.store(false, std::memory_order_release);
      return lab::make_leaf_error(lab::error_code::generic_error, ex.what());
    }

    return {};
  }

  // Enqueue fails only on allocation failure (see readerwriterqueue's
  // BlockingReaderWriterQueue::enqueue contract); recovery is not meaningful
  // at this layer, so the failure aborts the process.
  void post(task_t task) noexcept
  {
    LAB_ASSERT(task_queue_.enqueue(std::move(task)));
  }

  void stop() noexcept
  {
    running_.store(false, std::memory_order_release);
  }

  void join()
  {
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  [[nodiscard]] bool running() const noexcept
  {
    return running_.load(std::memory_order_acquire);
  }

private:
  void run()
  {
    // IMPROVEMENT: pin this to a cpu so that each event loop runs in a dedicated core
    set_current_thread_name(config_.thread_name);
    LAB_LOG_INFO("event loop started: {}", config_.thread_name);

    // Pick the loop body once at startup so we don't re-check the strategy
    // on every iteration.
    lab::match(
      config_.idle_strategy,
      [this](const timed_wait_idle& strategy) { run_timed_wait(strategy.duration); },
      [this](const busy_spin_idle&) { run_busy_spin(); });

    LAB_LOG_INFO("event loop stopped: {}", config_.thread_name);
  }

  void run_timed_wait(std::chrono::microseconds idle_wait)
  {
    task_t task{[] {}};

    while (running_.load(std::memory_order_acquire)) {
      bool did_work = false;

      while (task_queue_.try_dequeue(task)) {
        task();
        did_work = true;
      }

      for (auto& poller : pollers_) {
        did_work = poller() || did_work;
      }

      if (!did_work && task_queue_.wait_dequeue_timed(task, idle_wait)) {
        task();
      }
    }

    drain();
  }

  void run_busy_spin()
  {
    task_t task{[] {}};

    while (running_.load(std::memory_order_acquire)) {
      while (task_queue_.try_dequeue(task)) {
        task();
      }

      for (auto& poller : pollers_) {
        poller();
      }
    }

    drain();
  }

  // Precondition: the producer has been stopped, so the remaining tasks in
  // the queue are the final batch to execute before exit.
  void drain()
  {
    task_t task{[] {}};
    while (task_queue_.try_dequeue(task)) {
      task();
    }
  }

  static void set_current_thread_name(std::string_view name)
  {
    if (name.empty()) {
      return;
    }

    // Linux thread names: 15 bytes plus terminator.
    std::string truncated{name.substr(0, 15)};
    if (pthread_setname_np(pthread_self(), truncated.c_str()) != 0) {
      LAB_LOG_WARN("failed to set thread name: {}", name);
    }
  }

  event_loop_config config_;
  std::jthread thread_;
  std::atomic<bool> running_{false};
  blocking_concurrent_queue<task_t> task_queue_;
  std::vector<poller_t> pollers_;
};

} // namespace lab

#endif /* LAB_EVENT_LOOP_HPP */
