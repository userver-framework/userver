#include <cache/cache_update_trait.hpp>

#include <engine/async.hpp>
#include <logging/log.hpp>
#include <testsuite/cache_control.hpp>
#include <tracing/tracer.hpp>

namespace cache {

void CacheUpdateTrait::Update(UpdateType update_type) {
  std::lock_guard<engine::Mutex> lock(update_mutex_);

  if (AllowedUpdateTypes() == AllowedUpdateTypes::kOnlyFull &&
      update_type == UpdateType::kIncremental) {
    update_type = UpdateType::kFull;
  }

  DoUpdate(update_type);
}

CacheUpdateTrait::CacheUpdateTrait(CacheConfig&& config,
                                   testsuite::CacheControl& cache_control,
                                   const std::string& name)
    // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
    : static_config_(std::move(config)),
      config_(static_config_),
      name_(name),
      is_running_(false),
      cache_control_(cache_control) {}

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
  return config_.allowed_update_types;
}

void CacheUpdateTrait::StartPeriodicUpdates(utils::Flags<Flag> flags) {
  if (is_running_.exchange(true)) {
    return;
  }

  // CacheInvalidatorHolder is created here to achieve that cache invalidators
  // are registered in the order of cache component dependency.
  // We exploit the fact that StartPeriodicUpdates is called at the end
  // of all concrete cache component constructors.
  cache_invalidator_holder_ =
      std::make_unique<testsuite::CacheInvalidatorHolder>(cache_control_,
                                                          *this);

  try {
    tracing::Span span("first-update/" + name_);

    if (!(flags & Flag::kNoFirstUpdate) || !IsPeriodicUpdateEnabled()) {
      // ignore kNoFirstUpdate if !IsPeriodicUpdateEnabled()
      // because some components require caches to be updated at least once

      // Force first update, do it synchronously
      try {
        DoPeriodicUpdate();
      } catch (const std::exception& e) {
        bool fail = !static_config_.allow_first_update_failure;
        LOG_ERROR() << "Failed to update cache " << name_
                    << " for the first time"
                    << (fail ? "" : ", leaving it empty");
        if (fail) throw;
      }
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

void CacheUpdateTrait::SetConfig(const boost::optional<CacheConfig>& config) {
  std::lock_guard<engine::Mutex> lock(update_mutex_);
  config_ = config.value_or(static_config_);
  update_task_.SetSettings(GetPeriodicTaskSettings());
}

void CacheUpdateTrait::DoPeriodicUpdate() {
  std::lock_guard<engine::Mutex> lock(update_mutex_);

  auto update_type = UpdateType::kFull;
  // first update is always full
  if (last_update_ != std::chrono::system_clock::time_point{}) {
    switch (AllowedUpdateTypes()) {
      case AllowedUpdateTypes::kOnlyFull:
        update_type = UpdateType::kFull;
        break;
      case AllowedUpdateTypes::kOnlyIncremental:
        update_type = UpdateType::kIncremental;
        break;
      case AllowedUpdateTypes::kFullAndIncremental:
        const auto steady_now = std::chrono::steady_clock::now();
        update_type =
            steady_now - last_full_update_ < config_.full_update_interval
                ? UpdateType::kIncremental
                : UpdateType::kFull;
        break;
    }
  }

  DoUpdate(update_type);
}

void CacheUpdateTrait::AssertPeriodicUpdateStarted() {
  UASSERT_MSG(is_running_.load(), "Cache " + name_ +
                                      " has been constructed without calling "
                                      "StartPeriodicUpdates(), call it in ctr");
}

void CacheUpdateTrait::DoUpdate(UpdateType update_type) {
  const auto steady_now = std::chrono::steady_clock::now();
  const auto update_type_str =
      update_type == UpdateType::kFull ? "full" : "incremental";

  UpdateStatisticsScope stats(GetStatistics(), update_type);
  LOG_INFO() << "Updating cache update_type=" << update_type_str
             << " name=" << name_;

  const auto system_now = std::chrono::system_clock::now();
  Update(update_type, last_update_, system_now, stats);
  LOG_INFO() << "Updated cache update_type=" << update_type_str
             << " name=" << name_;

  last_update_ = system_now;
  if (update_type == UpdateType::kFull) {
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

}  // namespace cache
