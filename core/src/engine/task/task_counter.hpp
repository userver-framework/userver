#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/aggregated_values.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskCounter final {
 public:
  class Token final {
   public:
    explicit Token(TaskCounter& counter) : counter_(counter) {
      ++counter_.tasks_alive_;
      ++counter_.tasks_created_;
    }
    ~Token() { --counter_.tasks_alive_; }

    Token(const Token&) = delete;
    Token(Token&&) = delete;
    Token& operator=(const Token&) = delete;
    Token& operator=(Token&&) = delete;

   private:
    TaskCounter& counter_;
  };

  class CoroToken final {
   public:
    explicit CoroToken(TaskCounter& counter) : counter_(&counter) {
      ++counter_->tasks_running_;
    }
    ~CoroToken() {
      if (counter_) --counter_->tasks_running_;
    }

    CoroToken(const CoroToken&) = delete;

    CoroToken(CoroToken&& other) noexcept
        : counter_(std::exchange(other.counter_, nullptr)) {}

    CoroToken& operator=(const CoroToken&) = delete;

    CoroToken& operator=(CoroToken&& rhs) noexcept {
      counter_ = std::exchange(rhs.counter_, nullptr);
      return *this;
    }

   private:
    TaskCounter* counter_;
  };

  ~TaskCounter() { UASSERT(!tasks_alive_); }

  template <typename Rep, typename Period>
  void WaitForExhaustion(
      const std::chrono::duration<Rep, Period>& check_period) const {
    while (tasks_alive_ > 0) {
      std::this_thread::sleep_for(check_period);
    }
  }

  size_t GetCurrentValue() const { return tasks_alive_; }

  size_t GetCreatedTasks() const { return tasks_created_; }

  size_t GetRunningTasks() const { return tasks_running_; }

  size_t GetCancelledTasks() const { return tasks_cancelled_; }

  size_t GetCancelledTasksOverload() const { return tasks_cancelled_overload_; }

  size_t GetTasksOverload() const { return tasks_overload_; }

  size_t GetTasksOverloadSensor() const { return tasks_overload_sensor_; }

  size_t GetTasksNoOverloadSensor() const { return tasks_no_overload_sensor_; }

  size_t GetTaskSwitchFast() const { return tasks_switch_fast_; }

  size_t GetTaskSwitchSlow() const { return tasks_switch_slow_; }

  size_t GetSpuriousWakeups() const { return spurious_wakeups_; }

  void AccountTaskCancel() noexcept { tasks_cancelled_++; }

  void AccountTaskCancelOverload() noexcept { tasks_cancelled_overload_++; }

  void AccountTaskOverload() noexcept { tasks_overload_++; }

  void AccountTaskOverloadSensor() { tasks_overload_sensor_++; }

  void AccountTaskNoOverloadSensor() { tasks_no_overload_sensor_++; }

  void AccountTaskSwitchFast() { tasks_switch_fast_++; }

  void AccountTaskSwitchSlow() { tasks_switch_slow_++; }

  void AccountSpuriousWakeup() { spurious_wakeups_++; }

  void AccountTaskExecution(std::chrono::microseconds us) {
    task_processor_profiler_timings_.Add(us.count(), 1);
  }

  const auto& GetTaskExecutionTimings() const {
    return task_processor_profiler_timings_;
  }

 private:
  std::atomic<size_t> tasks_alive_{0};
  std::atomic<size_t> tasks_created_{0};
  std::atomic<size_t> tasks_running_{0};
  std::atomic<size_t> tasks_cancelled_{0};
  std::atomic<size_t> tasks_switch_fast_{0};
  std::atomic<size_t> tasks_switch_slow_{0};
  std::atomic<size_t> spurious_wakeups_{0};
  std::atomic<size_t> tasks_cancelled_overload_{0};
  std::atomic<size_t> tasks_overload_{0};

  std::atomic<size_t> tasks_overload_sensor_{0};
  std::atomic<size_t> tasks_no_overload_sensor_{0};

  utils::statistics::AggregatedValues<25> task_processor_profiler_timings_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
