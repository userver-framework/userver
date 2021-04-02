#pragma once

/// @file cache/cache_update_trait.hpp
/// @brief @copybrief cache::CacheUpdateTrait

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <concurrent/variable.hpp>
#include <engine/mutex.hpp>
#include <engine/task/task_with_result.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/periodic_task.hpp>

#include <cache/cache_config.hpp>
#include <cache/cache_statistics.hpp>
#include <cache/dump/config.hpp>
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

class EmptyCacheError final : public std::runtime_error {
 public:
  explicit EmptyCacheError(const std::string& cache_name);
};

/// Base class for periodically updated caches
class CacheUpdateTrait {
 public:
  /// @brief Forces a cache update of specified type
  /// @throws If `Update` throws
  void Update(UpdateType update_type);

  /// @brief Forces the cache to read from a dump synchronously
  /// @throws dump::Error If the cache failed to read a cache dump
  void ReadDumpSyncDebug();

  /// @brief Forces the cache to write a dump synchronously
  /// @throws dump::Error If the dump should be written, but an error occurred
  void WriteDumpSyncDebug();

  const std::string& Name() const { return name_; }

 protected:
  virtual ~CacheUpdateTrait();

  CacheUpdateTrait(const components::ComponentConfig& config,
                   const components::ComponentContext& context);

  /// @warning This constructor must not be used directly, except for unit tests
  /// within userver
  CacheUpdateTrait(const Config& config, std::string name,
                   testsuite::CacheControl& cache_control,
                   const std::optional<dump::Config>& dump_config,
                   std::unique_ptr<dump::OperationsFactory> dump_rw_factory,
                   engine::TaskProcessor* fs_task_processor);

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

  /// @{
  /// @brief Updates cache config
  /// @note If no config is set, uses static default (from config.yaml).
  void SetConfig(const std::optional<ConfigPatch>& patch);

  void SetConfig(const std::optional<dump::ConfigPatch>& patch);
  /// @}

  /// Get a snapshot of current config
  rcu::ReadablePtr<Config> GetConfig() const;

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
  CacheUpdateTrait(const components::ComponentConfig& config,
                   const components::ComponentContext& context,
                   const std::optional<dump::Config>& dump_config);

  virtual void Cleanup() = 0;

  virtual void GetAndWrite(dump::Writer& writer) const;

  virtual void ReadAndSet(dump::Reader& reader);

  void DoPeriodicUpdate();

  struct UpdateData;
  struct DumpData;

  /// @throws If `Update` throws
  void DoUpdate(UpdateType type, UpdateData& update);

  enum class DumpType { kHonorDumpInterval, kForced };

  bool ShouldDump(DumpType type, UpdateData& update, DumpData& dump,
                  const dump::Config& dump_config);

  /// @throws On dump failure
  void DoDump(dump::TimePoint update_time, ScopeTime& scope, DumpData& dump);

  enum class DumpOperation { kNewDump, kBumpTime };

  void DumpAsync(DumpOperation operation_type, UpdateData& update,
                 DumpData& dump);

  /// @throws If `type == kForced`, and the cache is not ready to write a dump
  void DumpAsyncIfNeeded(DumpType type, UpdateData& update);

  /// @returns `true` on success
  bool LoadFromDump();

  utils::PeriodicTask::Settings GetPeriodicTaskSettings(const Config& config);

 private:
  Statistics statistics_;
  const Config static_config_;
  rcu::Variable<Config> config_;
  testsuite::CacheControl& cache_control_;
  const std::string name_;
  const bool periodic_update_enabled_;
  std::atomic<bool> is_running_;
  utils::PeriodicTask update_task_;
  utils::PeriodicTask cleanup_task_;
  bool force_next_update_full_;
  utils::Flags<utils::PeriodicTask::Flags> periodic_task_flags_;
  std::atomic<bool> cache_modified_;
  std::unique_ptr<testsuite::CacheInvalidatorHolder> cache_invalidator_holder_;

  struct UpdateData final {
    engine::TaskWithResult<void> dump_task;
    dump::TimePoint last_update;
    dump::TimePoint last_modifying_update;
    std::chrono::steady_clock::time_point last_full_update;
  };
  concurrent::Variable<UpdateData> update_;

  // TODO TAXICOMMON-3613 extract into a separate public class
  struct DumpData final {
    DumpData(const dump::Config& static_config,
             std::unique_ptr<dump::OperationsFactory> rw_factory,
             engine::TaskProcessor& fs_task_processor);

    const dump::Config static_config;
    rcu::Variable<dump::Config> config;
    std::unique_ptr<dump::OperationsFactory> rw_factory;
    engine::TaskProcessor& fs_task_processor;
    utils::FastPimpl<dump::DumpManager, 240, 8> dumper;
    std::atomic<dump::TimePoint> last_dumped_update;
  };
  std::optional<DumpData> dump_;
};

}  // namespace cache
