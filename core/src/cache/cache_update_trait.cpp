#include <cache/cache_update_trait.hpp>

#include <engine/async.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>

namespace components {

void CacheUpdateTrait::Update(cache::UpdateType update_type) {
  std::lock_guard<engine::Mutex> lock(update_mutex_);

  if (AllowedUpdateTypes() == cache::AllowedUpdateTypes::kOnlyFull &&
      update_type == cache::UpdateType::kIncremental) {
    update_type = cache::UpdateType::kFull;
  }

  // This will be removed with TAXIDATA-1333.
  // We introduced a change that must be supported in both Userver and Uservices
  // so the tests for Userver against unmodified Uservices wont pass.
  if (IsLegacyInvalidateLogicEnabled()) {
    const auto update_type_str =
        update_type == cache::UpdateType::kFull ? "full" : "incremental";

    cache::UpdateStatisticsScope stats(GetStatistics(), update_type);
    LOG_INFO() << "Updating cache type=" << update_type_str
               << " name=" << name_;
    Update(update_type, std::chrono::system_clock::time_point(),
           std::chrono::system_clock::now(), stats);
    LOG_INFO() << "Updated cache type=" << update_type_str << " name=" << name_;
  } else {
    DoUpdate(update_type);
  }
}

CacheUpdateTrait::CacheUpdateTrait(cache::CacheConfig&& config,
                                   const std::string& name)
    // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
    : static_config_(std::move(config)),
      config_(static_config_),
      name_(name),
      is_running_(false) {}

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

cache::AllowedUpdateTypes CacheUpdateTrait::AllowedUpdateTypes() const {
  return config_.allowed_update_types;
}

void CacheUpdateTrait::StartPeriodicUpdates(utils::Flags<Flag> flags) {
  if (is_running_.exchange(true)) {
    return;
  }

  try {
    tracing::Span span("first-update/" + name_);

    if (!(flags & Flag::kNoFirstUpdate) || !IsPeriodicUpdateEnabled()) {
      // ignore kNoFirstUpdate if !IsPeriodicUpdateEnabled()
      // because some components require caches to be updated at least once

      // Force first update, do it synchronously
      DoPeriodicUpdate();
    }

    if (IsPeriodicUpdateEnabled()) {
      update_task_.Start("update-task/" + name_, GetPeriodicTaskSettings(),
                         [this]() { DoPeriodicUpdate(); });
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

  if (update_task_.IsRunning()) {
    try {
      update_task_.Stop();
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Exception in update task: " << ex << ". Component name '"
                  << name_ << "'";
    }
  }
}

void CacheUpdateTrait::SetConfig(
    const boost::optional<cache::CacheConfig>& config) {
  std::lock_guard<engine::Mutex> lock(update_mutex_);
  config_ = config.value_or(static_config_);
  update_task_.SetSettings(GetPeriodicTaskSettings());
}

void CacheUpdateTrait::DoPeriodicUpdate() {
  std::lock_guard<engine::Mutex> lock(update_mutex_);

  cache::UpdateType update_type;
  switch (AllowedUpdateTypes()) {
    case cache::AllowedUpdateTypes::kOnlyFull:
      update_type = cache::UpdateType::kFull;
      break;
    case cache::AllowedUpdateTypes::kOnlyIncremental:
      update_type = last_update_ == std::chrono::system_clock::time_point()
                        ? cache::UpdateType::kFull  // first update
                        : cache::UpdateType::kIncremental;
      break;
    case cache::AllowedUpdateTypes::kFullAndIncremental:
      const auto steady_now = std::chrono::steady_clock::now();
      update_type =
          steady_now - last_full_update_ < config_.full_update_interval
              ? cache::UpdateType::kIncremental
              : cache::UpdateType::kFull;
      break;
  }

  DoUpdate(update_type);
}

void CacheUpdateTrait::DoUpdate(cache::UpdateType update_type) {
  const auto steady_now = std::chrono::steady_clock::now();
  const auto update_type_str =
      update_type == cache::UpdateType::kFull ? "full" : "incremental";

  cache::UpdateStatisticsScope stats(GetStatistics(), update_type);
  LOG_INFO() << "Updating cache update_type=" << update_type_str
             << " name=" << name_;

  const auto system_now = std::chrono::system_clock::now();
  Update(update_type, last_update_, system_now, stats);
  LOG_INFO() << "Updated cache update_type=" << update_type_str
             << " name=" << name_;

  last_update_ = system_now;
  if (update_type == cache::UpdateType::kFull) {
    last_full_update_ = steady_now;
  }
}

utils::PeriodicTask::Settings CacheUpdateTrait::GetPeriodicTaskSettings()
    const {
  return utils::PeriodicTask::Settings(config_.update_interval,
                                       config_.update_jitter,
                                       {utils::PeriodicTask::Flags::kChaotic,
                                        utils::PeriodicTask::Flags::kCritical});
}

}  // namespace components
