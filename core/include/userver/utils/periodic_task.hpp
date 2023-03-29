#pragma once

/// @file userver/utils/periodic_task.hpp
/// @brief @copybrief utils::PeriodicTask

#include <chrono>
#include <functional>
#include <optional>
#include <string>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/testsuite/periodic_task_control.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_concurrency
///
/// @brief Task that periodically runs a user callback. Callback is started
/// after the previous callback execution is finished every `period + A - B`,
/// where:
/// * `A` is `+/- distribution * rand(0.0, 1.0)` if Flags::kChaotic flag is set,
///   otherwise is `0`;
/// * `B` is the time of previous callback execution if Flags::kStrong flag is
///   set, otherwise is `0`;
///
/// TaskProcessor to execute the callback and many other options are specified
/// in PeriodicTask::Settings.
class PeriodicTask final {
 public:
  enum class Flags {
    /// None of the below flags
    kNone = 0,
    /// Immediately call a function
    kNow = 1 << 0,
    /// Account function call time as a part of wait period
    kStrong = 1 << 1,
    /// Randomize wait period (+-25% by default)
    kChaotic = 1 << 2,
    /// Use `engine::Task::Importance::kCritical` flag
    /// @note Although this periodic task cannot be cancelled due to
    /// system overload, it's cancelled upon calling `Stop`.
    /// Subtasks that may be spawned in the callback
    /// are not critical by default and may be cancelled as usual.
    kCritical = 1 << 4,
  };

  /// Configuration parameters for PeriodicTask.
  struct Settings final {
    static constexpr uint8_t kDistributionPercent = 25;

    constexpr /*implicit*/ Settings(
        std::chrono::milliseconds period, utils::Flags<Flags> flags = {},
        logging::Level span_level = logging::Level::kInfo)
        : Settings(period, kDistributionPercent, flags, span_level) {}

    constexpr Settings(std::chrono::milliseconds period,
                       std::chrono::milliseconds distribution,
                       utils::Flags<Flags> flags = {},
                       logging::Level span_level = logging::Level::kInfo)
        : period(period),
          distribution(distribution),
          flags(flags),
          span_level(span_level) {
      UASSERT(distribution <= period);
    }

    constexpr Settings(std::chrono::milliseconds period,
                       uint8_t distribution_percent,
                       utils::Flags<Flags> flags = {},
                       logging::Level span_level = logging::Level::kInfo)
        : Settings(period, period * distribution_percent / 100, flags,
                   span_level) {
      UASSERT(distribution_percent <= 100);
    }

    template <class Rep, class Period>
    constexpr /*implicit*/ Settings(std::chrono::duration<Rep, Period> period)
        : Settings(period, kDistributionPercent, {}, logging::Level::kInfo) {}

    // Note: Tidy requires us to explicitly initialize these fields, although
    // the initializers are never used.

    /// @brief Period for the task execution. Task is repeated every
    /// `(period +/- distribution) - time of previous execution`
    std::chrono::milliseconds period{};

    /// @brief Jitter for task repetitions. If kChaotic is set in `flags`
    /// the task is repeated every
    /// `(period +/- distribution) - time of previous execution`
    std::chrono::milliseconds distribution{};

    /// @brief Used instead of `period` in case of exception, if set.
    std::optional<std::chrono::milliseconds> exception_period;

    /// @brief Flags that control the behavior of PeriodicTask.
    utils::Flags<Flags> flags{};

    /// @brief tracing::Span that measures each execution of the task
    /// uses this logging level.
    logging::Level span_level{logging::Level::kInfo};

    /// @brief TaskProcessor to execute the task. If nullptr then the
    /// PeriodicTask::Start() calls engine::current_task::GetTaskProcessor()
    /// to get the TaskProcessor.
    engine::TaskProcessor* task_processor{nullptr};
  };

  /// Signature of the task to be executed each period.
  using Callback = std::function<void()>;

  /// Default constructor that does nothing.
  PeriodicTask();

  PeriodicTask(PeriodicTask&&) = delete;
  PeriodicTask(const PeriodicTask&) = delete;

  /// Constructs the periodic task and calls Start()
  PeriodicTask(std::string name, Settings settings, Callback callback);

  /// Stops the periodic execution of previous task and starts the periodic
  /// execution of the new task.
  void Start(std::string name, Settings settings, Callback callback);

  ~PeriodicTask();

  /// @brief Stops the PeriodicTask. If a Step() is in progress, cancels it and
  /// waits for its completion.
  /// @warning PeriodicTask must be stopped before the callback becomes invalid.
  /// E.g. if your class X stores PeriodicTask and the callback is class' X
  /// method, you have to explicitly stop PeriodicTask in ~X() as after ~X()
  /// exits the object is destroyed and using X's 'this' in callback is UB.
  void Stop() noexcept;

  /// Set all settings except flags. All flags must be set at the start.
  void SetSettings(Settings settings);

  /// Force next DoStep() iteration. It is guaranteed that there is at least one
  /// call to DoStep() during SynchronizeDebug() execution. DoStep() is executed
  /// as usual in the PeriodicTask's task (NOT in current task).
  /// @param preserve_span run periodic task current span if true. It's here for
  /// backward compatibility with existing tests. Will be removed in
  /// TAXIDATA-1499.
  /// @returns true if task was successfully executed.
  /// @note On concurrent invocations, the task is guaranteed to be invoked
  /// serially, one time after another.
  bool SynchronizeDebug(bool preserve_span = false);

  /// Skip Step() calls from loop until ResumeDebug() is called. If DoStep()
  /// is executing, wait its completion, for a potentially long time.
  /// The purpose is to control task execution from tests.
  void SuspendDebug();

  /// Stop skipping Step() calls from loop. Returns without waiting for
  /// DoStep() call. The purpose is to control task execution from tests.
  void ResumeDebug();

  /// Checks if a periodic task (not a single iteration only) is running.
  /// It may be in a callback execution or sleeping between callbacks.
  bool IsRunning() const;

  /// Make this periodic task available for testsuite. Testsuite provides a way
  /// to call it directly from testcase.
  void RegisterInTestsuite(
      testsuite::PeriodicTaskControl& periodic_task_control);

  /// Get current settings. Note that they might become stale very quickly.
  Settings GetCurrentSettings() const;

 private:
  enum class SuspendState { kRunning, kSuspended };

  void DoStart();

  void Run();

  bool Step();

  bool StepDebug(bool preserve_span);

  bool DoStep();

  std::chrono::milliseconds MutatePeriod(std::chrono::milliseconds period);

  rcu::Variable<std::string> name_;
  Callback callback_;
  engine::TaskWithResult<void> task_;
  rcu::Variable<Settings> settings_;
  engine::SingleConsumerEvent changed_event_;

  // For kNow only
  engine::Mutex step_mutex_;
  std::atomic<SuspendState> suspend_state_;

  std::optional<testsuite::PeriodicTaskRegistrationHolder> registration_holder_;
};

}  // namespace utils

USERVER_NAMESPACE_END
