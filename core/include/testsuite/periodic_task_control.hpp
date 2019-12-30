#pragma once

/// @file testsuite/periodic_task_control.hpp
/// @brief @copybrief testsuite::PeriodicTaskControl

#include <unordered_map>

#include <engine/mutex.hpp>

namespace utils {
class PeriodicTask;
}  // namespace utils

namespace testsuite {

/// @brief Periodic task control interface for testsuite
/// @details All methods are coro-safe.
class PeriodicTaskControl final {
 public:
  bool RunPeriodicTask(const std::string& name);

 private:
  friend class PeriodicTaskRegistrationHolder;

  void RegisterPeriodicTask(const std::string& name, utils::PeriodicTask& task);

  void UnregisterPeriodicTask(const std::string& name,
                              utils::PeriodicTask& task);

  utils::PeriodicTask& FindPeriodicTask(const std::string& name);

  engine::Mutex mutex_;
  std::unordered_map<std::string, utils::PeriodicTask&> periodic_tasks_;
};

/// RAII helper for testsuite registration
class PeriodicTaskRegistrationHolder {
 public:
  PeriodicTaskRegistrationHolder(PeriodicTaskControl& periodic_task_control,
                                 std::string name, utils::PeriodicTask& task);
  ~PeriodicTaskRegistrationHolder();

  PeriodicTaskRegistrationHolder(const PeriodicTaskRegistrationHolder&) =
      delete;
  PeriodicTaskRegistrationHolder(PeriodicTaskRegistrationHolder&&) = delete;
  PeriodicTaskRegistrationHolder& operator=(
      const PeriodicTaskRegistrationHolder&) = delete;
  PeriodicTaskRegistrationHolder& operator=(PeriodicTaskRegistrationHolder&&) =
      delete;

 private:
  PeriodicTaskControl& periodic_task_control_;
  std::string name_;
  utils::PeriodicTask& task_;
};

}  // namespace testsuite
