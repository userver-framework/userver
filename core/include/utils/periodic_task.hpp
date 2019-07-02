#pragma once

#include <random>

#include <engine/condition_variable.hpp>
#include <engine/deadline.hpp>
#include <engine/task/task_with_result.hpp>
#include <rcu/rcu.hpp>
#include <tracing/span.hpp>
#include <utils/assert.hpp>
#include <utils/flags.hpp>

namespace utils {

/**
 * @brief Task that periodically run a user callback.
 */
class PeriodicTask final {
 public:
  enum class Flags {
    kNone = 0,
    /// Immediatelly call a function
    kNow = 1 << 0,
    /// Account function call time as a part of wait period
    kStrong = 1 << 1,
    /// Randomize wait period (+-25% by default)
    kChaotic = 1 << 2,
    /// Use `engine::Task::Importance::kCritical` flag
    /// @note Although this periodic task cannot be canceled due to
    /// system overload, it's canceled upon calling `Stop`.
    /// Subtasks that may be spawned in the callback
    /// are not critical by default and may be canceled as usual.
    kCritical = 1 << 4,
  };

  struct Settings {
    static constexpr uint8_t kDistributionPercent = 25;

    Settings(std::chrono::milliseconds period, utils::Flags<Flags> flags = {},
             logging::Level span_level = logging::Level::kInfo)
        : Settings(period, kDistributionPercent, flags, span_level) {}

    Settings(std::chrono::milliseconds period,
             std::chrono::milliseconds distribution,
             utils::Flags<Flags> flags = {},
             logging::Level span_level = logging::Level::kInfo)
        : period(period),
          distribution(distribution),
          flags(flags),
          span_level(span_level) {
      UASSERT(distribution <= period);
    }

    Settings(std::chrono::milliseconds period, uint8_t distribution_percent,
             utils::Flags<Flags> flags = {},
             logging::Level span_level = logging::Level::kInfo)
        : Settings(period, period * distribution_percent / 100, flags,
                   span_level) {
      UASSERT(distribution_percent <= 100);
    }

    std::chrono::milliseconds period;
    std::chrono::milliseconds distribution;
    /// Used instead of period in case of exception, if set.
    boost::optional<std::chrono::milliseconds> exception_period;
    utils::Flags<Flags> flags;
    logging::Level span_level;
  };

  using Callback = std::function<void()>;

  PeriodicTask();
  PeriodicTask(PeriodicTask&&) = delete;
  PeriodicTask(const PeriodicTask&) = delete;

  PeriodicTask(std::string name, Settings settings, Callback callback);

  void Start(std::string name, Settings settings, Callback callback);

  /* A user has to stop it *before* the callback becomes invalid.
   * E.g. if your class X stores PeriodicTask and the callback is class' X
   * method, you have to explicitly stop PeriodicTask in ~X() as after ~X()
   * exits the object is destroyed and using X's 'this' in callback is UB.
   */
  ~PeriodicTask() {
    UASSERT(!IsRunning());
    Stop();
  }

  void Stop() noexcept;

  void SetSettings(Settings settings);

  /// Force next DoStep() iteration. It is guaranteed that there is at least one
  /// call to DoStep() during SynchronizeDebug() execution. DoStep() is executed
  /// as usual in the PeriodicTask's task (NOT in current task).
  void SynchronizeDebug();

  /// Checks if a periodic task (not a single iteration only) is running.
  /// It may be in a callback execution or sleeping between callbacks.
  bool IsRunning() const;

 private:
  void SleepUntil(engine::Deadline::TimePoint tp);

  void DoStart();

  void Run();

  bool Step();

  bool DoStep();

  std::chrono::milliseconds MutatePeriod(std::chrono::milliseconds period);

  void WaitForFirstStep();

 private:
  std::string name_;
  Callback callback_;
  engine::TaskWithResult<void> task_;
  rcu::Variable<Settings> settings_;
  std::minstd_rand rand_;  // default seed is OK

  // For kNow only
  engine::ConditionVariable start_cv_;
  engine::Mutex start_mutex_;
  std::atomic<bool> started_;
};

}  // namespace utils
