#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include <utils/periodic_task.hpp>

#include "cache_config.hpp"

namespace components {

class CacheUpdateTrait {
 public:
  enum class UpdateType { kFull, kIncremental };

  void UpdateFull(tracing::Span&& span);

 protected:
  CacheUpdateTrait(CacheConfig&& config, const std::string& name);
  virtual ~CacheUpdateTrait();

  void StartPeriodicUpdates();
  void StopPeriodicUpdates();

 private:
  virtual void Update(UpdateType type,
                      const std::chrono::system_clock::time_point& last_update,
                      const std::chrono::system_clock::time_point& now,
                      tracing::Span&& span) = 0;

  void DoPeriodicUpdate(tracing::Span&& span);

  CacheConfig config_;
  const std::string name_;
  std::atomic<bool> is_running_;
  utils::PeriodicTask update_task_;

  std::chrono::system_clock::time_point last_update_;
  std::chrono::steady_clock::time_point last_full_update_;
};

}  // namespace components
