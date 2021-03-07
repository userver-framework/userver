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
#include <cache/dump/factory.hpp>
#include <cache/dump/operations.hpp>
#include <cache/update_type.hpp>

namespace testsuite {
class CacheControl;
class CacheInvalidatorHolder;
}  // namespace testsuite

namespace cache {

/// @cond
namespace dump {

class DumpManager;

using TimePoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

}  // namespace dump

[[noreturn]] void ThrowDumpUnimplemented(const std::string& name);
/// @endcond

class EmptyCacheError : public std::runtime_error {
 public:
  explicit EmptyCacheError(const std::string& cache_name);
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

  CacheUpdateTrait(const CacheConfigStatic& config,
                   testsuite::CacheControl& cache_control, std::string name,
                   engine::TaskProcessor& fs_task_processor);

  CacheUpdateTrait(const CacheConfigStatic& config,
                   std::unique_ptr<dump::OperationsFactory> dump_rw_factory,
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

  formats::json::Value ExtendStatistics();

  /// @brief Updates cache config
  /// @note If no config is set, uses static default (from config.yaml).
  void SetConfig(const std::optional<CacheConfig>& config);

  /// Get a snapshot of current config
  rcu::ReadablePtr<CacheConfigStatic> GetConfig() { return config_.Read(); }

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

 private:
  virtual void Cleanup() = 0;

  virtual void GetAndWrite(dump::Writer& writer) const;

  virtual void ReadAndSet(dump::Reader& reader);

  void DoPeriodicUpdate();

  struct UpdateData;

  /// @throws If `Update` throws
  void DoUpdate(UpdateType type, UpdateData& update);

  enum class DumpType { kHonorDumpInterval, kForced };

  bool ShouldDump(DumpType type, UpdateData& update,
                  const CacheConfigStatic& config);

  /// @returns `true` on success
  bool DoDump(dump::TimePoint update_time, ScopeTime& scope);

  enum class DumpOperation { kNewDump, kBumpTime };

  void DumpAsync(DumpOperation operation_type, UpdateData& update);

  void DumpAsyncIfNeeded(DumpType type, UpdateData& update,
                         const CacheConfigStatic& config);

  /// @returns `true` on success
  bool LoadFromDump(const CacheConfigStatic& config);

  dump::TimePoint GetLastDumpedUpdate();

  utils::PeriodicTask::Settings GetPeriodicTaskSettings(
      const CacheConfigStatic& config);

 private:
  Statistics statistics_;
  const CacheConfigStatic static_config_;
  rcu::Variable<CacheConfigStatic> config_;
  testsuite::CacheControl& cache_control_;
  const std::string name_;
  engine::TaskProcessor& fs_task_processor_;
  const bool periodic_update_enabled_;
  std::atomic<bool> is_running_;
  utils::PeriodicTask update_task_;
  utils::PeriodicTask cleanup_task_;
  bool force_next_update_full_;
  utils::Flags<utils::PeriodicTask::Flags> periodic_task_flags_;
  std::atomic<bool> cache_modified_;

  std::atomic<dump::TimePoint> last_dumped_update_;
  std::unique_ptr<dump::OperationsFactory> dump_rw_factory_;
  utils::FastPimpl<dump::DumpManager, 240, 8> dumper_;
  std::unique_ptr<testsuite::CacheInvalidatorHolder> cache_invalidator_holder_;

  struct UpdateData {
    engine::TaskWithResult<void> dump_task;
    dump::TimePoint last_update;
    dump::TimePoint last_modifying_update;
    std::chrono::steady_clock::time_point last_full_update;
  };
  concurrent::Variable<UpdateData> update_;
};

}  // namespace cache
