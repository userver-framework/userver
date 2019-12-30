#pragma once

/// @file cache/cache_update_trait.hpp
/// @brief @copybrief cache::CacheUpdateTrait

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <cache/cache_statistics.hpp>
#include <cache/update_type.hpp>
#include <engine/mutex.hpp>
#include <utils/periodic_task.hpp>

#include "cache_config.hpp"

namespace testsuite {
class CacheControl;
class CacheInvalidatorHolder;
}  // namespace testsuite

namespace cache {

/// Base class for periodically updated caches
class CacheUpdateTrait {
 public:
  /// Forces cache update of specified type
  void Update(UpdateType update_type);

  std::string GetName() const { return name_; }

 protected:
  CacheUpdateTrait(CacheConfig&& config, testsuite::CacheControl& cache_control,
                   const std::string& name);
  virtual ~CacheUpdateTrait();

  /// Update types configured for the cache
  AllowedUpdateTypes AllowedUpdateTypes() const;

  /// Periodic update flags
  enum class Flag {
    kNone = 0,
    kNoFirstUpdate = 1 << 0,  ///< Disable initial update on start
  };

  /// Starts periodic updates
  void StartPeriodicUpdates(utils::Flags<Flag> flags = {});

  /// @brief Stops periodic updates
  /// @warning Should be called in destructor of derived class.
  void StopPeriodicUpdates();

  Statistics& GetStatistics() { return statistics_; }

  /// @brief Updates cache config
  /// @note If no config is set, uses static default (from config.yaml).
  void SetConfig(const boost::optional<CacheConfig>& config);

 protected:
  /// Can be called for force update. There is a mutex lock inside, so it's safe
  /// to call this method in parallel with periodic cache updates or other force
  /// updates.
  void DoPeriodicUpdate();

  virtual bool IsPeriodicUpdateEnabled() const { return true; }

  void AssertPeriodicUpdateStarted();

 private:
  void DoUpdate(UpdateType type);

  virtual void Update(UpdateType type,
                      const std::chrono::system_clock::time_point& last_update,
                      const std::chrono::system_clock::time_point& now,
                      UpdateStatisticsScope& stats_scope) = 0;

  utils::PeriodicTask::Settings GetPeriodicTaskSettings() const;

  Statistics statistics_;
  engine::Mutex update_mutex_;
  const CacheConfig static_config_;
  CacheConfig config_;
  const std::string name_;
  std::atomic<bool> is_running_;
  utils::PeriodicTask update_task_;
  testsuite::CacheControl& cache_control_;
  std::unique_ptr<testsuite::CacheInvalidatorHolder> cache_invalidator_holder_;

  std::chrono::system_clock::time_point last_update_;
  std::chrono::steady_clock::time_point last_full_update_;
};

}  // namespace cache
