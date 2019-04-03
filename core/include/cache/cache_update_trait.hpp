#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include <cache/cache_statistics.hpp>
#include <cache/update_type.hpp>
#include <engine/mutex.hpp>
#include <utils/periodic_task.hpp>

#include "cache_config.hpp"

namespace components {

class CacheUpdateTrait {
 public:
  void UpdateFull();

 protected:
  CacheUpdateTrait(CacheConfig&& config, const std::string& name);
  virtual ~CacheUpdateTrait();

  enum class Flag {
    kNone = 0,
    kNoFirstUpdate = 1 << 0,
  };

  void StartPeriodicUpdates(utils::Flags<Flag> flags = {});
  void StopPeriodicUpdates();

  cache::Statistics& GetStatistics() { return statistics_; }

  /* If no config is set, use static default (from config.yaml) */
  void SetConfig(const boost::optional<CacheConfig>& config);

 private:
  virtual void Update(cache::UpdateType type,
                      const std::chrono::system_clock::time_point& last_update,
                      const std::chrono::system_clock::time_point& now,
                      cache::UpdateStatisticsScope& stats_scope) = 0;

  void DoPeriodicUpdate();

  utils::PeriodicTask::Settings GetPeriodicTaskSettings() const;

  cache::Statistics statistics_;
  engine::Mutex update_mutex_;
  const CacheConfig static_config_;
  CacheConfig config_;
  const std::string name_;
  std::atomic<bool> is_running_;
  utils::PeriodicTask update_task_;

  std::chrono::system_clock::time_point last_update_;
  std::chrono::steady_clock::time_point last_full_update_;
};

}  // namespace components
