#pragma once

#include <atomic>
#include <chrono>
#include <random>
#include <string>

#include <engine/async_task.hpp>
#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>

#include "component_base.hpp"
#include "component_config.hpp"

namespace components {

class UpdatingComponentBase : public ComponentBase {
 public:
  enum class UpdateType { kFull, kIncremental };

  const std::string& Name() const;

 protected:
  UpdatingComponentBase(const ComponentConfig& config, std::string name);

  void StartPeriodicUpdates();
  void StopPeriodicUpdates();

 private:
  virtual void Update(UpdateType type,
                      const std::chrono::system_clock::time_point& last_update,
                      const std::chrono::system_clock::time_point& now) = 0;

  void DoPeriodicUpdate();

  std::string name_;

  std::chrono::milliseconds update_interval_;
  std::chrono::milliseconds update_jitter_;
  std::chrono::milliseconds full_update_interval_;

  std::chrono::system_clock::time_point last_update_;
  std::chrono::steady_clock::time_point last_full_update_;

  engine::Mutex is_running_mutex_;
  engine::ConditionVariable is_running_cv_;
  std::atomic<bool> is_running_;

  std::default_random_engine jitter_generator_;
  engine::AsyncTask<void> update_task_;
};

}  // namespace components
