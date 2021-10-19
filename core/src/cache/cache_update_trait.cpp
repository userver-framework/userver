#include <userver/cache/cache_update_trait.hpp>

#include <userver/components/dump_configurator.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/taxi_config/source.hpp>
#include <userver/taxi_config/storage/component.hpp>
#include <userver/testsuite/cache_control.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/atomic.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/statistics/metadata.hpp>

#include <dump/dump_locator.hpp>
#include <userver/dump/factory.hpp>
#include <userver/testsuite/testsuite_support.hpp>

namespace cache {

namespace {

template <typename T>
T CheckNotNull(T ptr) {
  UINVARIANT(ptr, "This pointer must not be null");
  return std::move(ptr);
}

std::optional<dump::Config> ParseOptionalDumpConfig(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  if (!config.HasMember(dump::kDump)) return {};
  return dump::Config{
      config.Name(), config[dump::kDump],
      context.FindComponent<components::DumpConfigurator>().GetDumpRoot()};
}

engine::TaskProcessor& FindTaskProcessor(
    const components::ComponentContext& context, const Config& static_config) {
  return static_config.task_processor_name
             ? context.GetTaskProcessor(*static_config.task_processor_name)
             : engine::current_task::GetTaskProcessor();
}

std::optional<taxi_config::Source> FindTaxiConfig(
    const components::ComponentContext& context, const Config& static_config) {
  return static_config.config_updates_enabled
             ? std::optional{context.FindComponent<components::TaxiConfig>()
                                 .GetSource()}
             : std::nullopt;
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

  utils::CriticalAsync(task_processor_, "update-task/" + name_, [&] {
    DoUpdate(update_type);
  }).Get();
}

CacheUpdateTrait::CacheUpdateTrait(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : CacheUpdateTrait(config, context,
                       ParseOptionalDumpConfig(config, context)) {}

CacheUpdateTrait::CacheUpdateTrait(
    const components::ComponentConfig& config,
    const components::ComponentContext& context,
    const std::optional<dump::Config>& dump_config)
    : CacheUpdateTrait(config, context, dump_config,
                       Config{config, dump_config}) {}

CacheUpdateTrait::CacheUpdateTrait(
    const components::ComponentConfig& config,
    const components::ComponentContext& context,
    const std::optional<dump::Config>& dump_config, const Config& static_config)
    : CacheUpdateTrait(
          static_config, config.Name(),
          FindTaskProcessor(context, static_config),
          FindTaxiConfig(context, static_config),
          context.FindComponent<components::StatisticsStorage>().GetStorage(),
          context.FindComponent<components::TestsuiteSupport>()
              .GetCacheControl(),
          dump_config,
          dump_config ? dump::CreateOperationsFactory(*dump_config, context)
                      : nullptr,
          dump_config
              ? &context.GetTaskProcessor(dump_config->fs_task_processor)
              : nullptr,
          context.FindComponent<components::TestsuiteSupport>()
              .GetDumpControl()) {}

CacheUpdateTrait::CacheUpdateTrait(
    const Config& config, std::string name,
    engine::TaskProcessor& task_processor,
    std::optional<taxi_config::Source> config_source,
    utils::statistics::Storage& statistics_storage,
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
      task_processor_(task_processor),
      is_running_(false),
      first_update_attempted_(false),
      periodic_task_flags_{utils::PeriodicTask::Flags::kChaotic,
                           utils::PeriodicTask::Flags::kCritical},
      cache_modified_(false),
      dumper_(dump_config ? std::optional<dump::Dumper>(
                                std::in_place, *dump_config,
                                CheckNotNull(std::move(dump_rw_factory)),
                                *CheckNotNull(fs_task_processor),
                                *CheckNotNull(config_source),
                                statistics_storage, dump_control, *this)
                          : std::nullopt) {
  statistics_holder_ = statistics_storage.RegisterExtender(
      "cache." + Name(), [this](auto&) { return ExtendStatistics(); });

  if (config.config_updates_enabled) {
    config_subscription_ =
        CheckNotNull(config_source)
            ->UpdateAndListen(this, "cache." + Name(),
                              &CacheUpdateTrait::OnConfigUpdate);
  }
}

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
    if (dump_time) {
      last_update_ = *dump_time;
      forced_update_type_ = config->first_update_type == FirstUpdateType::kFull
                                ? UpdateType::kFull
                                : UpdateType::kIncremental;
    }

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

    if (dump_time && config->first_update_type ==
                         FirstUpdateType::kIncrementalThenAsyncFull) {
      forced_update_type_ = UpdateType::kFull;
      periodic_task_flags_ |= utils::PeriodicTask::Flags::kNow;
    }

    if (config->is_strong_period) {
      periodic_task_flags_ |= utils::PeriodicTask::Flags::kStrong;
    }

    if (periodic_update_enabled_) {
      update_task_.Start("update-task/" + name_,
                         GetPeriodicTaskSettings(*config),
                         [this] { DoPeriodicUpdate(); });

      utils::PeriodicTask::Settings cleanup_settings(config->cleanup_interval);
      cleanup_settings.span_level = logging::Level::kNone;
      cleanup_settings.task_processor = &task_processor_;

      cleanup_task_.Start("rcu-cleanup-task/" + name_, cleanup_settings,
                          [this] {
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

  if (dumper_) {
    engine::TaskCancellationBlocker blocker;
    dumper_->CancelWriteTaskAndWait();
  }
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

  return builder.ExtractValue();
}

void CacheUpdateTrait::OnConfigUpdate(const taxi_config::Snapshot& config) {
  const auto patch = utils::FindOptional(config[kCacheConfigSet], Name());
  config_.Assign(patch ? static_config_.MergeWith(*patch) : static_config_);
  const auto new_config = config_.Read();
  update_task_.SetSettings(GetPeriodicTaskSettings(*new_config));
  cleanup_task_.SetSettings({new_config->cleanup_interval});
}

rcu::ReadablePtr<Config> CacheUpdateTrait::GetConfig() const {
  return config_.Read();
}

UpdateType CacheUpdateTrait::NextUpdateType(const Config& config) {
  auto forced_update_type = std::exchange(forced_update_type_, {});
  if (forced_update_type) return *forced_update_type;

  if (last_update_ == dump::TimePoint{}) return UpdateType::kFull;

  switch (config.allowed_update_types) {
    case AllowedUpdateTypes::kOnlyFull:
      return UpdateType::kFull;
    case AllowedUpdateTypes::kOnlyIncremental:
      return UpdateType::kIncremental;
    case AllowedUpdateTypes::kFullAndIncremental:
      const auto steady_now = utils::datetime::SteadyNow();
      return steady_now - last_full_update_ < config.full_update_interval
                 ? UpdateType::kIncremental
                 : UpdateType::kFull;
  }
}

void CacheUpdateTrait::DoPeriodicUpdate() {
  std::lock_guard lock(update_mutex_);
  const auto config = GetConfig();

  const auto is_first_update = !std::exchange(first_update_attempted_, true);
  if (!config->updates_enabled && !is_first_update) {
    LOG_INFO() << "Periodic updates are disabled for cache " << Name();
    return;
  }

  const auto update_type = NextUpdateType(*config);
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

engine::TaskProcessor& CacheUpdateTrait::GetCacheTaskProcessor() const {
  return task_processor_;
}

void CacheUpdateTrait::GetAndWrite(dump::Writer&) const {
  dump::ThrowDumpUnimplemented(name_);
}

void CacheUpdateTrait::ReadAndSet(dump::Reader&) {
  dump::ThrowDumpUnimplemented(name_);
}

void CacheUpdateTrait::DoUpdate(UpdateType update_type) {
  const auto steady_now = utils::datetime::SteadyNow();
  const auto now =
      std::chrono::round<dump::TimePoint::duration>(utils::datetime::Now());

  const auto update_type_str =
      update_type == UpdateType::kFull ? "full" : "incremental";
  tracing::Span::CurrentSpan().AddTag("update_type", update_type_str);

  UpdateStatisticsScope stats(GetStatistics(), update_type);
  LOG_INFO() << "Updating cache update_type=" << update_type_str
             << " name=" << name_;

  Update(update_type, last_update_, now, stats);
  LOG_INFO() << "Updated cache update_type=" << update_type_str
             << " name=" << name_;

  last_update_ = now;
  if (update_type == UpdateType::kFull) {
    last_full_update_ = steady_now;
  }
  if (dumper_) {
    dumper_->OnUpdateCompleted(now, cache_modified_.exchange(false)
                                        ? dump::UpdateType::kModified
                                        : dump::UpdateType::kAlreadyUpToDate);
  }
}

utils::PeriodicTask::Settings CacheUpdateTrait::GetPeriodicTaskSettings(
    const Config& config) {
  utils::PeriodicTask::Settings settings{
      config.update_interval, config.update_jitter, periodic_task_flags_};
  settings.task_processor = &task_processor_;
  return settings;
}

}  // namespace cache
