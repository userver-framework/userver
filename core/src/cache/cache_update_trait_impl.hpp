#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

#include <userver/components/component_fwd.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/cache_update_trait.hpp>
#include <userver/cache/update_type.hpp>
#include <userver/dump/dumper.hpp>
#include <userver/dump/operations.hpp>
#include <userver/testsuite/cache_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

struct CacheDependencies;
class CacheUpdateTrait;

class CacheUpdateTrait::Impl final {
 public:
  explicit Impl(CacheDependencies&& dependencies, CacheUpdateTrait& self);

  ~Impl();

  void InvalidateAsync(UpdateType update_type);

  void UpdateSyncDebug(UpdateType update_type);

  const std::string& Name() const;

  AllowedUpdateTypes GetAllowedUpdateTypes() const;

  void StartPeriodicUpdates(utils::Flags<CacheUpdateTrait::Flag> flags = {});

  void StopPeriodicUpdates();

  void AssertPeriodicUpdateStarted();

  void OnCacheModified();

  bool HasPreAssignCheck() const;

  rcu::ReadablePtr<Config> GetConfig() const;

  engine::TaskProcessor& GetCacheTaskProcessor() const;

 private:
  class DumpableEntityProxy final : public dump::DumpableEntity {
   public:
    explicit DumpableEntityProxy(CacheUpdateTrait& cache);

    void GetAndWrite(dump::Writer& writer) const override;

    void ReadAndSet(dump::Reader& reader) override;

   private:
    CacheUpdateTrait& cache_;
  };

  enum class FirstUpdateInvalidation { kNo, kYes, kFinished };

  UpdateType NextUpdateType(const Config& config);

  void DoPeriodicUpdate();

  void OnPeriodicUpdateFailure();

  // Throws if `Update` throws
  void DoUpdate(UpdateType type);

  utils::PeriodicTask::Settings GetPeriodicTaskSettings(const Config& config);

  void OnConfigUpdate(const dynamic_config::Snapshot& config);

  // Over-aligned members go first
  utils::PeriodicTask update_task_;
  utils::PeriodicTask cleanup_task_;
  std::optional<dump::Dumper> dumper_;

  CacheUpdateTrait& customized_trait_;
  impl::Statistics statistics_;
  const Config static_config_;
  rcu::Variable<Config> config_;
  testsuite::CacheControl& cache_control_;
  const std::string name_;
  const std::string update_task_name_;
  engine::TaskProcessor& task_processor_;
  const bool periodic_update_enabled_;
  std::atomic<bool> is_running_{false};
  bool first_update_attempted_{false};
  std::atomic<bool> cache_modified_{false};
  utils::Flags<utils::PeriodicTask::Flags> periodic_task_flags_;
  dump::TimePoint last_update_;
  std::chrono::steady_clock::time_point last_full_update_;
  engine::Mutex update_mutex_;
  DumpableEntityProxy dumpable_;
  std::uint64_t failed_updates_counter_{0};
  std::atomic<FirstUpdateInvalidation> first_update_invalidation_{
      FirstUpdateInvalidation::kNo};

  // `dump_first_update_type_` has the highest priority.
  // This means that if `dump_first_update_type_` is equal to `kIncremental` and
  // `force_full_update_` is true, the `kIncremental` update will be performed.
  std::optional<UpdateType> dump_first_update_type_;
  std::atomic<bool> force_full_update_{false};

  utils::statistics::Entry statistics_holder_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
  std::optional<testsuite::CacheInvalidatorHolder> cache_invalidator_holder_;
};

}  // namespace cache

USERVER_NAMESPACE_END
