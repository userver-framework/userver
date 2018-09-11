#pragma once

#include <random>

#include <engine/task/task_with_result.hpp>
#include <utils/flags.hpp>
#include <utils/swappingsmart.hpp>

namespace utils {

/**
 * @brief Task that periodically run a user callback.
 */
class PeriodicTask {
 public:
  enum class Flags {
    kNone = 0,
    /// Immediatelly call a function
    kNow = 1 << 0,
    /// Account function call time as a part of wait period
    kStrong = 1 << 1,
    /// Randomize wait period +-25%
    kChaotic = 1 << 2
  };

  struct Settings {
    Settings(std::chrono::milliseconds period, utils::Flags<Flags> flags = {})
        : period(period), flags(flags) {}

    std::chrono::milliseconds period;
    /// Used instead of period, if set.
    boost::optional<std::chrono::milliseconds> exception_period;
    utils::Flags<Flags> flags;
  };

  PeriodicTask() = default;
  PeriodicTask(PeriodicTask&&) = delete;
  PeriodicTask(const PeriodicTask&) = delete;

  PeriodicTask(std::string name, Settings settings,
               std::function<void()> callback);

  void Start(std::string name, Settings settings,
             std::function<void()> callback);

  /* A user has to stop it *before* the callback becomes invalid.
   * E.g. if your class X stores PeriodicTask and the callback is class' X
   * method, you have to explicitly stop PeriodicTask in ~X() as after ~X()
   * exits the object is destroyed and using X's 'this' in callback is UB.
   */
  ~PeriodicTask() {
    assert(task_.IsFinished());
    Stop();
  }

  void Stop();

  void SetSettings(Settings settings);

 private:
  void DoStart();

  void Run();

  std::chrono::milliseconds MutatePeriod(std::chrono::milliseconds period);

 private:
  std::string name_;
  std::function<void()> callback_;
  engine::TaskWithResult<void> task_;
  utils::SwappingSmart<Settings> settings_;
  std::minstd_rand rand_;  // default seed is OK
};

}  // namespace utils
