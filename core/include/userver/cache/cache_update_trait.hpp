#pragma once

/// @file userver/cache/cache_update_trait.hpp
/// @brief @copybrief cache::CacheUpdateTrait

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/taxi_config/config_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/update_type.hpp>
#include <userver/dump/dumper.hpp>
#include <userver/dump/operations.hpp>

namespace dump {
struct Config;
class OperationsFactory;
}  // namespace dump

namespace testsuite {
class CacheControl;
class CacheInvalidatorHolder;
class DumpControl;
}  // namespace testsuite

namespace cache {

class EmptyCacheError final : public std::runtime_error {
 public:
  explicit EmptyCacheError(const std::string& cache_name);
};

/// @ingroup userver_base_classes
///
/// Base class for periodically updated caches
class CacheUpdateTrait : public dump::DumpableEntity {
 public:
  /// @brief Forces a cache update of specified type
  /// @throws If `Update` throws
  void Update(UpdateType update_type);

  const std::string& Name() const { return name_; }

 protected:
  ~CacheUpdateTrait() override;

  CacheUpdateTrait(const components::ComponentConfig& config,
                   const components::ComponentContext& context);

  /// @warning This constructor must not be used directly, except for unit tests
  /// within userver
  CacheUpdateTrait(const Config& config, std::string name,
                   std::optional<taxi_config::Source> config_source,
                   utils::statistics::Storage& statistics_storage,
                   testsuite::CacheControl& cache_control,
                   const std::optional<dump::Config>& dump_config,
                   std::unique_ptr<dump::OperationsFactory> dump_rw_factory,
                   engine::TaskProcessor* fs_task_processor,
                   testsuite::DumpControl& dump_control);

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

  CacheUpdateTrait(const components::ComponentConfig& config,
                   const components::ComponentContext& context,
                   const std::optional<dump::Config>& dump_config,
                   const Config& static_config);

  virtual void Cleanup() = 0;

  void GetAndWrite(dump::Writer& writer) const override;

  void ReadAndSet(dump::Reader& reader) override;

  UpdateType NextUpdateType(const Config& config);

  void DoPeriodicUpdate();

  /// @throws If `Update` throws
  void DoUpdate(UpdateType type);

  utils::PeriodicTask::Settings GetPeriodicTaskSettings(const Config& config);

  void OnConfigUpdate(const taxi_config::Snapshot& config);

  formats::json::Value ExtendStatistics();

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
  bool first_update_attempted_;
  std::optional<UpdateType> forced_update_type_;
  utils::Flags<utils::PeriodicTask::Flags> periodic_task_flags_;
  std::atomic<bool> cache_modified_;
  dump::TimePoint last_update_;
  std::chrono::steady_clock::time_point last_full_update_;
  engine::Mutex update_mutex_;
  std::optional<dump::Dumper> dumper_;

  utils::statistics::Entry statistics_holder_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
  std::unique_ptr<testsuite::CacheInvalidatorHolder> cache_invalidator_holder_;
};

}  // namespace cache
