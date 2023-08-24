#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <concurrent/impl/interference_shield.hpp>
#include <userver/utils/fixed_array.hpp>
#include <userver/utils/statistics/rate_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskCounter final {
  using Rate = utils::statistics::Rate;

 public:
  class Token;
  class CoroToken;

  explicit TaskCounter(std::size_t thread_count);

  ~TaskCounter();

  void WaitForExhaustion() const noexcept;

  // May return 'true' when there are no tasks alive (due to races).
  // Never returns 'false' when there are tasks alive.
  bool MayHaveTasksAlive() const noexcept;

  Rate GetCreatedTasks() const noexcept;

  Rate GetDestroyedTasks() const noexcept;

  Rate GetStartedTasks() const noexcept;

  Rate GetStoppedTasks() const noexcept;

  Rate GetCancelledTasks() const noexcept;

  Rate GetCancelledTasksOverload() const noexcept;

  Rate GetTasksOverload() const noexcept;

  Rate GetTasksOverloadSensor() const noexcept;

  Rate GetTasksNoOverloadSensor() const noexcept;

  Rate GetTaskSwitchFast() const noexcept;

  Rate GetTaskSwitchSlow() const noexcept;

  Rate GetSpuriousWakeups() const noexcept;

  void AccountTaskCancel() noexcept;

  void AccountTaskCancelOverload() noexcept;

  void AccountTaskOverload() noexcept;

  void AccountTaskOverloadSensor() noexcept;

  void AccountTaskNoOverloadSensor() noexcept;

  void AccountTaskSwitchFast() noexcept;

  void AccountTaskSwitchSlow() noexcept;

  void AccountSpuriousWakeup() noexcept;

 private:
  // Counters that may be mutated from outside the bound TaskProcessor.
  enum class GlobalCounterId : std::size_t {
    kCreated,
    kDestroyed,

    kCountersSize,
  };

  // Counters that are only mutated from the bound TaskProcessor.
  enum class LocalCounterId : std::size_t {
    kCreated = static_cast<std::size_t>(GlobalCounterId::kCreated),
    kDestroyed = static_cast<std::size_t>(GlobalCounterId::kDestroyed),

    kStarted,
    kStopped,
    kCancelled,
    kSwitchFast,
    kSwitchSlow,
    kSpuriousWakeups,
    kCancelOverload,
    kOverload,
    kOverloadSensor,
    kNoOverloadSensor,

    kCountersSize,
  };

  static constexpr auto kLocalCountersSize =
      static_cast<std::size_t>(LocalCounterId::kCountersSize);
  static constexpr auto kGlobalCountersSize =
      static_cast<std::size_t>(GlobalCounterId::kCountersSize);

  using Counter = utils::statistics::RateCounter;

  using LocalCounterPack = concurrent::impl::InterferenceShield<
      std::array<Counter, kLocalCountersSize>>;

  using GlobalCounterPack =
      std::array<concurrent::impl::InterferenceShield<Counter>,
                 kGlobalCountersSize>;

  Rate GetApproximate(LocalCounterId) const noexcept;

  Rate GetApproximate(GlobalCounterId) const noexcept;

  void Increment(LocalCounterId) noexcept;

  void Increment(GlobalCounterId) noexcept;

  GlobalCounterPack global_counters_;
  utils::FixedArray<LocalCounterPack> local_counters_;
};

class TaskCounter::Token final {
 public:
  explicit Token(TaskCounter& counter) noexcept;

  Token(const Token&) = delete;
  Token(Token&&) = delete;
  Token& operator=(const Token&) = delete;
  Token& operator=(Token&&) = delete;
  ~Token();

 private:
  TaskCounter& counter_;
};

class TaskCounter::CoroToken final {
 public:
  explicit CoroToken(TaskCounter& counter) noexcept;

  CoroToken(const CoroToken&) = delete;
  CoroToken& operator=(const CoroToken&) = delete;
  CoroToken(CoroToken&& other) noexcept;
  CoroToken& operator=(CoroToken&& rhs) noexcept;
  ~CoroToken();

 private:
  TaskCounter* counter_;
};

void SetLocalTaskCounterData(TaskCounter& counter, std::size_t thread_id);

}  // namespace engine::impl

USERVER_NAMESPACE_END
