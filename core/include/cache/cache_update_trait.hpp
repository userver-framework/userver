#pragma once

/// @file cache/cache_update_trait.hpp
/// @brief @copybrief cache::CacheUpdateTrait

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <concurrent/variable.hpp>
#include <engine/mutex.hpp>
#include <engine/task/task_with_result.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/periodic_task.hpp>

#include <cache/cache_config.hpp>
#include <cache/cache_statistics.hpp>
#include <cache/update_type.hpp>

namespace testsuite {
class CacheControl;
class CacheInvalidatorHolder;
}  // namespace testsuite

namespace cache {

/// @cond
class Dumper;

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;
/// @endcond

class EmptyCacheError : public std::runtime_error {
 public:
  explicit EmptyCacheError(const std::string& cache_name)
      : std::runtime_error("Cache " + cache_name + " is empty") {}
};

/// Base class for periodically updated caches
class CacheUpdateTrait {
 public:
  /// @brief Forces a cache update of specified type
  /// @throws If `Update` throws
  void Update(UpdateType update_type);

  /// Forces a synchronous cache dump
  void DumpSyncDebug();

  const std::string& Name() const { return name_; }

 protected:
  virtual ~CacheUpdateTrait();

  CacheUpdateTrait(CacheConfigStatic&& config,
                   testsuite::CacheControl& cache_control, std::string name,
                   engine::TaskProcessor& fs_task_processor);

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
  void SetConfig(const std::optional<CacheConfig>& config);

  /// Get a snapshot of current config
  rcu::ReadablePtr<CacheConfigStatic> GetConfig() { return config_.Read(); }

  /// @brief Override to indicate that `Serialize` and `Parse` are implemented
  /// @see Serialize, Parse
  virtual bool IsDumpEnabled() const { return false; }

  void AssertPeriodicUpdateStarted();

  /// Called in `CachingComponentBase::Set` during update to indicate
  /// that the cached data has been modified
  void OnCacheModified();

 protected:
  /// @brief Must override in a subclass
  /// @note Must update statistics using `stats_scope`, and call
  /// `CachingComponentBase::Set` if the cached data has changed
  /// @throws Any `std::exception` on error
  virtual void Update(UpdateType type,
                      const std::chrono::system_clock::time_point& last_update,
                      const std::chrono::system_clock::time_point& now,
                      UpdateStatisticsScope& stats_scope) = 0;

  /// @brief Override to implement storing cache contents to a cache dump
  /// @note Return null if there is no data to dump
  /// @see IsDumpEnabled, Parse
  virtual std::optional<std::string> GetAndSerialize() const;

  /// @brief Override to implement loading cache contents from a cache dump
  /// @see IsDumpEnabled, Serialize
  virtual void ParseAndSet(const std::string&);

 private:
  virtual void Cleanup() = 0;

  void DoPeriodicUpdate();

  struct UpdateData;

  /// @throws If `Update` throws
  void DoUpdate(UpdateType type, UpdateData& update);

  enum class DumpType { kHonorDumpInterval, kForced };

  bool ShouldDump(DumpType type, UpdateData& update,
                  const CacheConfigStatic& config);

  /// @note If `contents` is empty, then just bumps the update time of the dump
  void DumpAsync(std::optional<std::string> contents, UpdateData& update);

  void DumpAsyncIfNeeded(DumpType type, UpdateData& update,
                         const CacheConfigStatic& config);

  /// @returns `true` on success
  bool LoadFromDump(UpdateData& update);

  TimePoint GetLastDumpedUpdate();

  static utils::PeriodicTask::Settings GetPeriodicTaskSettings(
      const CacheConfigStatic& config);

 private:
  Statistics statistics_;
  const CacheConfigStatic static_config_;
  rcu::Variable<CacheConfigStatic> config_;
  testsuite::CacheControl& cache_control_;
  const std::string name_;
  const bool periodic_update_enabled_;
  std::atomic<bool> is_running_;
  utils::PeriodicTask update_task_;
  utils::PeriodicTask cleanup_task_;
  std::atomic<bool> cache_modified_;
  std::atomic<TimePoint> last_dumped_update_;
  utils::FastPimpl<Dumper, 240, 8> dumper_;

  struct UpdateData {
    engine::TaskWithResult<void> dump_task;
    std::unique_ptr<testsuite::CacheInvalidatorHolder> cache_invalidator_holder;
    TimePoint last_update;
    TimePoint last_modifying_update;
    std::chrono::steady_clock::time_point last_full_update;
  };
  concurrent::Variable<UpdateData> update_;
};

}  // namespace cache
