#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <cache/cache_statistics.hpp>
#include <cache/update_type.hpp>
#include <engine/mutex.hpp>
#include <utils/periodic_task.hpp>

#include "cache_config.hpp"

namespace server {
class CacheInvalidatorHolder;
}  // namespace server

namespace components {
class TestsuiteSupport;

class CacheUpdateTrait {
 public:
  void Update(cache::UpdateType update_type);

  std::string GetName() const { return name_; }

 protected:
  CacheUpdateTrait(cache::CacheConfig&& config,
                   components::TestsuiteSupport& testsuite_support,
                   const std::string& name);
  virtual ~CacheUpdateTrait();

  cache::AllowedUpdateTypes AllowedUpdateTypes() const;

  enum class Flag {
    kNone = 0,
    kNoFirstUpdate = 1 << 0,
  };

  void StartPeriodicUpdates(utils::Flags<Flag> flags = {});
  void StopPeriodicUpdates();

  cache::Statistics& GetStatistics() { return statistics_; }

  /* If no config is set, use static default (from config.yaml) */
  void SetConfig(const boost::optional<cache::CacheConfig>& config);

 protected:
  /// Can be called for force update. There is a mutex lock inside, so it's safe
  /// to call this method in parallel with periodic cache updates or other force
  /// updates.
  void DoPeriodicUpdate();

  virtual bool IsPeriodicUpdateEnabled() const { return true; }

  void AssertPeriodicUpdateStarted();

 private:
  void DoUpdate(cache::UpdateType type);

  virtual void Update(cache::UpdateType type,
                      const std::chrono::system_clock::time_point& last_update,
                      const std::chrono::system_clock::time_point& now,
                      cache::UpdateStatisticsScope& stats_scope) = 0;

  utils::PeriodicTask::Settings GetPeriodicTaskSettings() const;

  cache::Statistics statistics_;
  engine::Mutex update_mutex_;
  const cache::CacheConfig static_config_;
  cache::CacheConfig config_;
  const std::string name_;
  std::atomic<bool> is_running_;
  utils::PeriodicTask update_task_;
  components::TestsuiteSupport& testsuite_support_;
  std::unique_ptr<server::CacheInvalidatorHolder> cache_invalidator_holder_;

  std::chrono::system_clock::time_point last_update_;
  std::chrono::steady_clock::time_point last_full_update_;
};

}  // namespace components
