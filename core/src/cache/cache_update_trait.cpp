#include <cache/cache_update_trait.hpp>

#include <boost/filesystem/operations.hpp>

#include <logging/log.hpp>
#include <testsuite/cache_control.hpp>
#include <tracing/tracer.hpp>
#include <utils/assert.hpp>
#include <utils/async.hpp>
#include <utils/atomic.hpp>
#include <utils/statistics/metadata.hpp>

#include <dump/dump_locator.hpp>
#include <dump/factory.hpp>
#include <testsuite/testsuite_support.hpp>

namespace cache {

namespace {

template <typename T>
T CheckNotNull(T ptr) {
  YTX_INVARIANT(ptr, "This pointer must not be null");
  return std::move(ptr);
}

}  // namespace

EmptyCacheError::EmptyCacheError(const std::string& cache_name)
    : std::runtime_error("Cache " + cache_name + " is empty") {}

void CacheUpdateTrait::Update(UpdateType update_type) {
  std::lock_guard lock(update_mutex_);
  const auto config = GetConfig();

  if (config->allowed_update_types == AllowedUpdateTypes::kOnlyFull &&
      update_type == UpdateType::kIncremental) {
    update_type = UpdateType::kFull;
  }

  DoUpdate(update_type);
}

CacheUpdateTrait::CacheUpdateTrait(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : CacheUpdateTrait(config, context, dump::Config::ParseOptional(config)) {}

CacheUpdateTrait::CacheUpdateTrait(
    const components::ComponentConfig& config,
    const components::ComponentContext& context,
    const std::optional<dump::Config>& dump_config)
    : CacheUpdateTrait(Config{config, dump_config}, config.Name(),
                       context.FindComponent<components::TestsuiteSupport>()
                           .GetCacheControl(),
                       dump_config,
                       dump_config ? dump::CreateOperationsFactory(
                                         *dump_config, context, config.Name())
                                   : nullptr,
                       dump_config ? &context.GetTaskProcessor(
                                         dump_config->fs_task_processor)
                                   : nullptr,
                       context.FindComponent<components::TestsuiteSupport>()
                           .GetDumpControl()) {}

CacheUpdateTrait::CacheUpdateTrait(
    const Config& config, std::string name,
    testsuite::CacheControl& cache_control,
    const std::optional<dump::Config>& dump_config,
    std::unique_ptr<dump::OperationsFactory> dump_rw_factory,
    engine::TaskProcessor* fs_task_processor,
    testsuite::DumpControl& dump_control)
    : static_config_(config),
      config_(static_config_),
      cache_control_(cache_control),
      name_(std::move(name)),
      periodic_update_enabled_(
          cache_control.IsPeriodicUpdateEnabled(static_config_, name_)),
      is_running_(false),
      force_next_update_full_(false),
      periodic_task_flags_{utils::PeriodicTask::Flags::kChaotic,
                           utils::PeriodicTask::Flags::kCritical},
      cache_modified_(false),
      dumper_(dump_config
                  ? std::optional<dump::Dumper>(
                        std::in_place, *dump_config,
                        CheckNotNull(std::move(dump_rw_factory)),
                        *CheckNotNull(fs_task_processor), dump_control, *this)
                  : std::nullopt) {}

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

  const auto config = GetConfig();

  // CacheInvalidatorHolder is created here to achieve that cache invalidators
  // are registered in the order of cache component dependency.
  // We exploit the fact that StartPeriodicUpdates is called at the end
  // of all concrete cache component constructors.
  cache_invalidator_holder_ =
      std::make_unique<testsuite::CacheInvalidatorHolder>(cache_control_,
                                                          *this);

  try {
    const auto dump_time = dumper_ ? dumper_->ReadDump() : std::nullopt;
    if (dump_time) last_update_ = *dump_time;

    if ((!dump_time || config->first_update_mode != FirstUpdateMode::kSkip) &&
        (!(flags & Flag::kNoFirstUpdate) || !periodic_update_enabled_)) {
      // ignore kNoFirstUpdate if !periodic_update_enabled_
      // because some components require caches to be updated at least once

      // Force first update, do it synchronously
      tracing::Span span("first-update/" + name_);
      try {
        DoPeriodicUpdate();
      } catch (const std::exception& e) {
        if (dump_time &&
            config->first_update_mode != FirstUpdateMode::kRequired) {
          LOG_ERROR() << "Failed to update cache " << name_
                      << " after loading a cache dump, going on with the "
                         "contents loaded from the dump";
        } else if (static_config_.allow_first_update_failure) {
          LOG_ERROR() << "Failed to update cache " << name_
                      << " for the first time, leaving it empty";
        } else {
          LOG_ERROR() << "Failed to update cache " << name_
                      << " for the first time";
          throw;
        }
      }
    }

    // Without this clause, after loading a cache dump, no full updates will
    // ever be performed with kOnlyIncremental. This can be problematic in case
    // the data in the cache has been corrupted in some way. Even restarting
    // the service won't help. Solution: perform a single asynchronous full
    // update.
    if (dump_time &&
        config->allowed_update_types == AllowedUpdateTypes::kOnlyIncremental &&
        config->force_full_second_update) {
      force_next_update_full_ = true;
      periodic_task_flags_ |= utils::PeriodicTask::Flags::kNow;
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

  if (dumper_) dumper_->CancelWriteTaskAndWait();
}

formats::json::Value CacheUpdateTrait::ExtendStatistics() {
  auto& full = GetStatistics().full_update;
  auto& incremental = GetStatistics().incremental_update;
  const auto any = cache::CombineStatistics(full, incremental);

  formats::json::ValueBuilder builder;
  utils::statistics::SolomonLabelValue(builder, "cache_name");
  builder[cache::kStatisticsNameFull] = cache::StatisticsToJson(full);
  builder[cache::kStatisticsNameIncremental] =
      cache::StatisticsToJson(incremental);
  builder[cache::kStatisticsNameAny] = cache::StatisticsToJson(any);

  builder[cache::kStatisticsNameCurrentDocumentsCount] =
      GetStatistics().documents_current_count.load();

  if (dumper_) {
    builder[cache::kStatisticsNameDump] = dumper_->ExtendStatistics();
  }

  return builder.ExtractValue();
}

void CacheUpdateTrait::SetConfigPatch(const std::optional<ConfigPatch>& patch) {
  config_.Assign(patch ? static_config_.MergeWith(*patch) : static_config_);
  const auto new_config = config_.Read();
  update_task_.SetSettings(GetPeriodicTaskSettings(*new_config));
  cleanup_task_.SetSettings({new_config->cleanup_interval});
}

void CacheUpdateTrait::SetConfigPatch(
    const std::optional<dump::ConfigPatch>& patch) {
  if (dumper_) {
    dumper_->SetConfigPatch(patch);
  } else if (patch) {
    LOG_WARNING() << "Dynamic dump config is set for cache " << Name()
                  << ", but it doesn't support dumps";
  }
}

rcu::ReadablePtr<Config> CacheUpdateTrait::GetConfig() const {
  return config_.Read();
}

void CacheUpdateTrait::DoPeriodicUpdate() {
  std::lock_guard lock(update_mutex_);
  const auto config = GetConfig();

  // The update is full regardless of `update_type`:
  // - if the cache is empty, or
  // - if the update is forced to be full (see `StartPeriodicUpdates`)
  const bool force_full_update =
      std::exchange(force_next_update_full_, false) ||
      last_update_ == std::chrono::system_clock::time_point{};

  auto update_type = UpdateType::kFull;
  if (!force_full_update) {
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
            steady_now - last_full_update_ < config->full_update_interval
                ? UpdateType::kIncremental
                : UpdateType::kFull;
        break;
    }
  }

  try {
    DoUpdate(update_type);
    if (dumper_) dumper_->WriteDumpAsync();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Error while updating cache " << name_
                  << ". Reason: " << ex;
    if (dumper_) dumper_->WriteDumpAsync();
    throw;
  }
}

void CacheUpdateTrait::AssertPeriodicUpdateStarted() {
  UASSERT_MSG(is_running_.load(), "Cache " + name_ +
                                      " has been constructed without calling "
                                      "StartPeriodicUpdates(), call it in ctr");
}

void CacheUpdateTrait::OnCacheModified() { cache_modified_ = true; }

void CacheUpdateTrait::GetAndWrite(dump::Writer&) const {
  dump::ThrowDumpUnimplemented(name_);
}

void CacheUpdateTrait::ReadAndSet(dump::Reader&) {
  dump::ThrowDumpUnimplemented(name_);
}

void CacheUpdateTrait::DoUpdate(UpdateType update_type) {
  const auto steady_now = std::chrono::steady_clock::now();
  const auto update_type_str =
      update_type == UpdateType::kFull ? "full" : "incremental";
  tracing::Span::CurrentSpan().AddTag("update_type", update_type_str);

  UpdateStatisticsScope stats(GetStatistics(), update_type);
  LOG_INFO() << "Updating cache update_type=" << update_type_str
             << " name=" << name_;

  const dump::TimePoint system_now =
      std::chrono::time_point_cast<dump::TimePoint::duration>(
          std::chrono::system_clock::now());
  Update(update_type, last_update_, system_now, stats);
  LOG_INFO() << "Updated cache update_type=" << update_type_str
             << " name=" << name_;

  last_update_ = system_now;
  if (update_type == UpdateType::kFull) {
    last_full_update_ = steady_now;
  }
  if (dumper_) {
    dumper_->OnUpdateCompleted(system_now, cache_modified_.exchange(false));
  }
}

utils::PeriodicTask::Settings CacheUpdateTrait::GetPeriodicTaskSettings(
    const Config& config) {
  return utils::PeriodicTask::Settings{
      config.update_interval, config.update_jitter, periodic_task_flags_};
}

}  // namespace cache
