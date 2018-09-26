#include <components/cache_update_trait.hpp>

#include <cstdint>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace components {

void CacheUpdateTrait::UpdateFull() {
  Update(UpdateType::kFull, std::chrono::system_clock::time_point(),
         std::chrono::system_clock::now());
}

CacheUpdateTrait::CacheUpdateTrait(CacheConfig&& config,
                                   const std::string& name)
    : config_(std::move(config)), name_(name), is_running_(false) {}

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

void CacheUpdateTrait::StartPeriodicUpdates() {
  if (is_running_.exchange(true)) {
    return;
  }

  // Force first update and wait for it to finish
  engine::CriticalAsync([this] { DoPeriodicUpdate(); }).Get();

  update_task_.Start(name_ + "-update-task",
                     {config_.update_interval_,
                      config_.update_jitter_,
                      {utils::PeriodicTask::Flags::kChaotic,
                       utils::PeriodicTask::Flags::kCritical}},
                     [this] { DoPeriodicUpdate(); });
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

void CacheUpdateTrait::DoPeriodicUpdate() {
  const auto steady_now = std::chrono::steady_clock::now();
  auto update_type = UpdateType::kFull;
  if (last_full_update_ + config_.full_update_interval_ > steady_now) {
    update_type = UpdateType::kIncremental;
  }

  const auto system_now = std::chrono::system_clock::now();
  Update(update_type, last_update_, system_now);

  last_update_ = system_now;
  if (update_type == UpdateType::kFull) {
    last_full_update_ = steady_now;
  }
}

}  // namespace components
