#include <cache/cache_update_trait.hpp>

#include <cstdint>

#include <engine/async.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>

namespace components {

void CacheUpdateTrait::UpdateFull() {
  std::lock_guard<engine::Mutex> lock(update_mutex_);

  auto update_type = cache::UpdateType::kFull;
  cache::UpdateStatisticsScope stats(GetStatistics(), update_type);

  Update(update_type, std::chrono::system_clock::time_point(),
         std::chrono::system_clock::now(), stats);
}

CacheUpdateTrait::CacheUpdateTrait(CacheConfig&& config,
                                   const std::string& name)
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
    assert(false && "StopPeriodicUpdates() is not called");
  }
}

void CacheUpdateTrait::StartPeriodicUpdates(utils::Flags<Flag> flags) {
  if (is_running_.exchange(true)) {
    return;
  }

  try {
    tracing::Span span("first-update/" + name_);
    if (!(flags & Flag::kNoFirstUpdate)) {
      // Force first update, do it synchronously
      DoPeriodicUpdate();
    }

    update_task_.Start("update-task/" + name_, GetPeriodicTaskSettings(),
                       [this]() { DoPeriodicUpdate(); });
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
    LOG_ERROR() << "Exception in update task: " << ex.what()
                << ". Component name '" << name_ << "'";
  }
}

void CacheUpdateTrait::SetConfig(boost::optional<CacheConfig> config) {
  std::lock_guard<engine::Mutex> lock(update_mutex_);
  config_ = config.value_or(static_config_);
  update_task_.SetSettings(GetPeriodicTaskSettings());
}

void CacheUpdateTrait::DoPeriodicUpdate() {
  std::lock_guard<engine::Mutex> lock(update_mutex_);

  const auto steady_now = std::chrono::steady_clock::now();
  auto update_type = cache::UpdateType::kFull;
  if (last_full_update_ + config_.full_update_interval_ > steady_now) {
    update_type = cache::UpdateType::kIncremental;
  }

  cache::UpdateStatisticsScope stats(GetStatistics(), update_type);
  LOG_INFO() << "Updating cache name=" << name_;

  const auto system_now = std::chrono::system_clock::now();
  Update(update_type, last_update_, system_now, stats);
  LOG_INFO() << "Updated cache name=" << name_;

  last_update_ = system_now;
  if (update_type == cache::UpdateType::kFull) {
    last_full_update_ = steady_now;
  }
}

utils::PeriodicTask::Settings CacheUpdateTrait::GetPeriodicTaskSettings()
    const {
  return utils::PeriodicTask::Settings(config_.update_interval_,
                                       config_.update_jitter_,
                                       {utils::PeriodicTask::Flags::kChaotic,
                                        utils::PeriodicTask::Flags::kCritical});
}

}  // namespace components
