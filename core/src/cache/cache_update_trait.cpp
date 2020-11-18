#include <cache/cache_update_trait.hpp>

#include <logging/log.hpp>
#include <testsuite/cache_control.hpp>
#include <tracing/tracer.hpp>
#include <utils/assert.hpp>
#include <utils/async.hpp>
#include <utils/atomic.hpp>

#include <cache/dumper.hpp>

namespace cache {

void CacheUpdateTrait::Update(UpdateType update_type) {
  auto update = update_.Lock();
  const auto config = GetConfig();

  if (config->allowed_update_types == AllowedUpdateTypes::kOnlyFull &&
      update_type == UpdateType::kIncremental) {
    update_type = UpdateType::kFull;
  }

  DoUpdate(update_type, *update);

  if (!periodic_update_enabled_) {
    DumpAsyncIfNeeded(DumpType::kForced, *update, *config);
  } else {
    LOG_INFO() << "Deferring cache dump for cache " << name_
               << " to the next periodic update";
  }
}

CacheUpdateTrait::CacheUpdateTrait(CacheConfigStatic&& config,
                                   testsuite::CacheControl& cache_control,
                                   std::string name,
                                   engine::TaskProcessor& fs_task_processor)
    : static_config_(std::move(config)),
      config_(static_config_),
      cache_control_(cache_control),
      name_(std::move(name)),
      periodic_update_enabled_(
          cache_control.IsPeriodicUpdateEnabled(static_config_, name)),
      is_running_(false),
      cache_modified_(false),
      dumper_(CacheConfigStatic{static_config_}, fs_task_processor, name_) {}

CacheUpdateTrait::~CacheUpdateTrait() {
  if (is_running_.load()) {
    LOG_ERROR()
        << "CacheUpdateTrait is being destroyed while periodic update "
           "task is still running. "
           "Derived class has to call StopPeriodicUpdates() in destructor. "
        << "Component name '" << name_ << "'";
    // Don't crash in production
    UASSERT_MSG(false, "StopPeriodicUpdates() is not called");
  }
}

AllowedUpdateTypes CacheUpdateTrait::AllowedUpdateTypes() const {
  const auto config = config_.Read();
  return config->allowed_update_types;
}

void CacheUpdateTrait::StartPeriodicUpdates(utils::Flags<Flag> flags) {
  if (is_running_.exchange(true)) {
    return;
  }

  auto update = update_.Lock();
  const auto config = GetConfig();

  // CacheInvalidatorHolder is created here to achieve that cache invalidators
  // are registered in the order of cache component dependency.
  // We exploit the fact that StartPeriodicUpdates is called at the end
  // of all concrete cache component constructors.
  update->cache_invalidator_holder =
      std::make_unique<testsuite::CacheInvalidatorHolder>(cache_control_,
                                                          *this);

  try {
    const bool dump_loaded = LoadFromDump(*update);

    if (!dump_loaded &&
        (!(flags & Flag::kNoFirstUpdate) || !periodic_update_enabled_)) {
      // ignore kNoFirstUpdate if !IsPeriodicUpdateEnabled()
      // because some components require caches to be updated at least once

      // Force first update, do it synchronously
      tracing::Span span("first-update/" + name_);
      try {
        DoUpdate(UpdateType::kFull, *update);
        DumpAsyncIfNeeded(DumpType::kForced, *update, *config);
      } catch (const std::exception& e) {
        bool fail = !static_config_.allow_first_update_failure;
        LOG_ERROR() << "Failed to update cache " << name_
                    << " for the first time"
                    << (fail ? "" : ", leaving it empty");
        if (fail) throw;
      }
    }

    if (periodic_update_enabled_) {
      update_task_.Start("update-task/" + name_,
                         GetPeriodicTaskSettings(*config),
                         [this]() { DoPeriodicUpdate(); });
      cleanup_task_.Start(
          "cleanup-task/" + name_,
          utils::PeriodicTask::Settings(config->cleanup_interval), [this]() {
            tracing::Span::CurrentSpan().SetLocalLogLevel(
                logging::Level::kNone);
            config_.Cleanup();
            Cleanup();
          });
    }
  } catch (...) {
    is_running_ = false;  // update_task_ is not started, don't check it in dtr
    throw;
  }
}

void CacheUpdateTrait::StopPeriodicUpdates() {
  if (!is_running_.exchange(false)) {
    return;
  }

  try {
    update_task_.Stop();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Exception in update task of cache " << name_
                << ". Reason: " << ex;
  }

  try {
    cleanup_task_.Stop();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Exception in cleanup task of cache " << name_
                << ". Reason: " << ex;
  }

  auto update = update_.Lock();
  if (update->dump_task.IsValid()) {
    LOG_WARNING() << "Stopping a dump task of cache " << name_;
    try {
      update->dump_task.RequestCancel();
      update->dump_task.Wait();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Exception in dump task of cache " << name_
                  << ". Reason: " << ex;
    }
  }
}

void CacheUpdateTrait::SetConfig(const std::optional<CacheConfig>& config) {
  config_.Assign(config ? static_config_.MergeWith(*config) : static_config_);
  const auto new_config = config_.Read();
  update_task_.SetSettings(GetPeriodicTaskSettings(*new_config));
  cleanup_task_.SetSettings({new_config->cleanup_interval});
  dumper_->SetConfig(*new_config);
}

void CacheUpdateTrait::DoPeriodicUpdate() {
  auto update = update_.Lock();
  const auto config = GetConfig();

  auto update_type = UpdateType::kFull;
  // first update is always full
  if (update->last_update != std::chrono::system_clock::time_point{}) {
    switch (config->allowed_update_types) {
      case AllowedUpdateTypes::kOnlyFull:
        update_type = UpdateType::kFull;
        break;
      case AllowedUpdateTypes::kOnlyIncremental:
        update_type = UpdateType::kIncremental;
        break;
      case AllowedUpdateTypes::kFullAndIncremental:
        const auto steady_now = std::chrono::steady_clock::now();
        update_type =
            steady_now - update->last_full_update < config->full_update_interval
                ? UpdateType::kIncremental
                : UpdateType::kFull;
        break;
    }
  }

  try {
    DoUpdate(update_type, *update);
    DumpAsyncIfNeeded(DumpType::kHonorDumpInterval, *update, *config);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Error while updating cache " << name_
                  << ". Reason: " << ex;
    DumpAsyncIfNeeded(DumpType::kHonorDumpInterval, *update, *config);
    throw;
  }
}

void CacheUpdateTrait::AssertPeriodicUpdateStarted() {
  UASSERT_MSG(is_running_.load(), "Cache " + name_ +
                                      " has been constructed without calling "
                                      "StartPeriodicUpdates(), call it in ctr");
}

void CacheUpdateTrait::OnCacheModified() { cache_modified_ = true; }

std::optional<std::string> CacheUpdateTrait::GetAndSerialize() const {
  const auto message =
      fmt::format("Cache dump is unimplemented for cache {}", name_);
  UASSERT_MSG(false, message);
  throw std::logic_error(message);
}

void CacheUpdateTrait::ParseAndSet(const std::string&) {
  const auto message =
      fmt::format("Cache dump is unimplemented for cache {}", name_);
  UASSERT_MSG(false, message);
  throw std::logic_error(message);
}

void CacheUpdateTrait::DoUpdate(UpdateType update_type, UpdateData& update) {
  const auto steady_now = std::chrono::steady_clock::now();
  const auto update_type_str =
      update_type == UpdateType::kFull ? "full" : "incremental";
  tracing::Span::CurrentSpan().AddTag("update_type", update_type_str);

  UpdateStatisticsScope stats(GetStatistics(), update_type);
  LOG_INFO() << "Updating cache update_type=" << update_type_str
             << " name=" << name_;

  const TimePoint system_now =
      std::chrono::time_point_cast<TimePoint::duration>(
          std::chrono::system_clock::now());
  Update(update_type, update.last_update, system_now, stats);
  LOG_INFO() << "Updated cache update_type=" << update_type_str
             << " name=" << name_;

  update.last_update = system_now;
  if (cache_modified_.exchange(false)) {
    update.last_modifying_update = system_now;
  }
  if (update_type == UpdateType::kFull) {
    update.last_full_update = steady_now;
  }
}

bool CacheUpdateTrait::ShouldDump(DumpType type, UpdateData& update,
                                  const CacheConfigStatic& config) {
  if (!IsDumpEnabled()) {
    LOG_INFO() << "Cache dump has not been performed, because cache dumps are "
                  "disabled for cache "
               << name_;
    return false;
  }

  if (type == DumpType::kHonorDumpInterval &&
      GetLastDumpedUpdate() > update.last_update - config.min_dump_interval) {
    LOG_DEBUG() << "Skipped cache dump for cache " << name_
                << ", because dump interval has not passed yet";
    return false;
  }

  // Prevent concurrent cache dumps from accumulating
  // and slowing everything down.
  if (update.dump_task.IsValid() && !update.dump_task.IsFinished()) {
    LOG_INFO() << "Skipped cache dump for cache " << name_
               << ", because a previous dump operation is in progress";
    return false;
  }

  return true;
}

void CacheUpdateTrait::DumpAsync(std::optional<std::string> contents,
                                 UpdateData& update) {
  UASSERT_MSG(!update.dump_task.IsValid() || update.dump_task.IsFinished(),
              "Another cache dump task is already running");

  if (update.dump_task.IsFinished()) {
    try {
      update.dump_task.Get();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Unexpected error from the previous cache dump for cache "
                  << name_ << ". Reason: " << ex;
    }
  }

  update.dump_task = utils::Async(
      engine::current_task::GetTaskProcessor(), "cache-dump",
      [this, contents = std::move(contents),
       old_update_time = GetLastDumpedUpdate(),
       new_update_time = update.last_modifying_update]() mutable {
        const bool success =
            contents
                ? dumper_->WriteNewDump({std::move(*contents), new_update_time})
                : dumper_->BumpDumpTime(old_update_time, new_update_time);

        if (success) {
          last_dumped_update_ = new_update_time;
        }

        // While update.dump_task hasn't finished, dumper_.WriteNewDump
        // won't be called in parallel with dumper_.Cleanup
        dumper_->Cleanup();
      });
}

void CacheUpdateTrait::DumpAsyncIfNeeded(DumpType type, UpdateData& update,
                                         const CacheConfigStatic& config) {
  if (!ShouldDump(type, update, config)) return;

  if (GetLastDumpedUpdate() == update.last_modifying_update) {
    // If nothing has been updated since the last time, skip the serialization
    // and dump processes by just renaming the dump file.
    LOG_DEBUG() << "Skipped cache dump for cache " << name_
                << ", because nothing has been updated";
    DumpAsync(std::nullopt, update);
  } else {
    std::optional<std::string> contents;
    try {
      // Serialize cached data to string synchronously:
      // the cache must be locked in the process
      auto scope_time =
          tracing::Span::CurrentSpan().CreateScopeTime("serialize_dump");
      contents = GetAndSerialize();
    } catch (const EmptyCacheError& ex) {
      LOG_DEBUG() << ex;
      // Proceed, treating EmptyCacheError like returning nullopt
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Error while serializing a cache dump for cache " << name_
                  << ". Reason: " << ex;
      return;
    }

    if (!contents) {
      LOG_INFO() << "Skipped cache dump for cache " << name_
                 << ", because there is no data to dump";
      return;
    }
    DumpAsync(std::move(*contents), update);
  }
}

bool CacheUpdateTrait::LoadFromDump(UpdateData& update) {
  tracing::Span span("load-from-dump/" + name_);

  if (!IsDumpEnabled()) {
    LOG_INFO() << "Could not load a cache dump, because cache dumps are "
                  "disabled for cache "
               << name_;
    return false;
  }

  std::optional<DumpContents> latest_dump = dumper_->ReadLatestDump();
  if (!latest_dump) return false;

  try {
    ParseAndSet(latest_dump->contents);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Error while parsing a cache dump for cache " << name_
                << ". Reason: " << ex;
    return false;
  }

  LOG_INFO() << "Loaded a cache dump for cache " << name_;
  update.last_update = latest_dump->update_time;
  update.last_modifying_update = latest_dump->update_time;

  utils::AtomicMax(last_dumped_update_, latest_dump->update_time);
  return true;
}

TimePoint CacheUpdateTrait::GetLastDumpedUpdate() {
  return last_dumped_update_.load();
}

utils::PeriodicTask::Settings CacheUpdateTrait::GetPeriodicTaskSettings(
    const CacheConfigStatic& config) {
  return utils::PeriodicTask::Settings(config.update_interval,
                                       config.update_jitter,
                                       {utils::PeriodicTask::Flags::kChaotic,
                                        utils::PeriodicTask::Flags::kCritical});
}

}  // namespace cache
