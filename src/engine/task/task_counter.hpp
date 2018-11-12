#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

#include <utils/statistics.hpp>

namespace engine {
namespace impl {

class TaskCounter {
 public:
  class Token {
   public:
    explicit Token(TaskCounter& counter) : counter_(counter) {
      ++counter_.value_;
      ++counter_.tasks_created_;
    }
    ~Token() { --counter_.value_; }

    Token(const Token&) = delete;
    Token(Token&&) = delete;
    Token& operator=(const Token&) = delete;
    Token& operator=(Token&&) = delete;

   private:
    TaskCounter& counter_;
  };

  ~TaskCounter() { assert(!value_); }

  template <typename Rep, typename Period>
  void WaitForExhaustion(
      const std::chrono::duration<Rep, Period>& check_period) const {
    while (value_ > 0) {
      std::this_thread::sleep_for(check_period);
    }
  }

  size_t GetCurrentValue() const { return value_; }

  size_t GetCreatedTasks() const { return tasks_created_; }

  size_t GetCancelledTasks() const { return tasks_cancelled_; }

  size_t GetCancelledTasksOverload() const { return tasks_cancelled_overload_; }

  size_t GetTasksOverload() const { return tasks_overload_; }

  size_t GetTaskSwitchFast() const { return tasks_switch_fast_; }

  size_t GetTaskSwitchSlow() const { return tasks_switch_slow_; }

  size_t GetSpuriousWakeups() const { return spurious_wakeups_; }

  void AccountTaskCancel() { tasks_cancelled_++; }

  void AccountTaskCancelOverload() { tasks_cancelled_overload_++; }

  void AccountTaskOverload() { tasks_overload_++; }

  void AccountTaskSwitchFast() { tasks_switch_fast_++; }

  void AccountTaskSwitchSlow() { tasks_switch_slow_++; }

  void AccountSpuriousWakeup() { spurious_wakeups_++; }

#ifdef USERVER_PROFILER
  void AccountTaskExecution(std::chrono::microseconds us) {
    task_processor_profiler_timings_.Add(us.count(), 1);
  }

  const auto& GetTaskExecutionTimings() const {
    return task_processor_profiler_timings_;
  }
#endif  // USERVER_PROFILER

 private:
  std::atomic<size_t> value_{0};
  std::atomic<size_t> tasks_created_{0};
  std::atomic<size_t> tasks_cancelled_{0};
  std::atomic<size_t> tasks_switch_fast_{0};
  std::atomic<size_t> tasks_switch_slow_{0};
  std::atomic<size_t> spurious_wakeups_{0};
  std::atomic<size_t> tasks_cancelled_overload_{0};
  std::atomic<size_t> tasks_overload_{0};

#ifdef USERVER_PROFILER
  statistics::AggregatedValues<25> task_processor_profiler_timings_;
#endif
};

}  // namespace impl
}  // namespace engine
