#include "updating_component_base.hpp"

#include <cstdint>

#include <engine/async.hpp>
#include <logging/log.hpp>

namespace components {

namespace {

const uint64_t kDefaultUpdateIntervalMs = 5 * 60 * 1000;

uint64_t GetDefaultJitterMs(const std::chrono::milliseconds& interval) {
  return (interval / 10).count();
}

}  // namespace

const std::string& UpdatingComponentBase::Name() const { return name_; }

UpdatingComponentBase::UpdatingComponentBase(const ComponentConfig& config,
                                             std::string name)
    : name_(std::move(name)),
      update_interval_(
          config.ParseUint64("update_interval_ms", kDefaultUpdateIntervalMs)),
      update_jitter_(config.ParseUint64("update_jitter_ms",
                                        GetDefaultJitterMs(update_interval_))),
      full_update_interval_(config.ParseUint64("full_update_interval_ms", 0)),
      is_running_(false),
      jitter_generator_(std::random_device()()) {}

void UpdatingComponentBase::StartPeriodicUpdates() {
  if (is_running_.exchange(true)) {
    return;
  }

  update_task_future_ = engine::Async([this] {
    while (is_running_) {
      try {
        DoPeriodicUpdate();
      } catch (const std::exception& ex) {
        LOG_WARNING() << "Cannot update " << name_ << ": " << ex.what();
      }
      std::uniform_int_distribution<decltype(update_jitter_)::rep>
          jitter_distribution(-update_jitter_.count(), update_jitter_.count());
      decltype(update_jitter_) jitter(jitter_distribution(jitter_generator_));

      std::unique_lock<std::mutex> lock(is_running_mutex_);
      is_running_cv_.WaitFor(lock, update_interval_ + jitter,
                             [this] { return !is_running_; });
    }
  });
}

void UpdatingComponentBase::StopPeriodicUpdates() {
  {
    std::lock_guard<std::mutex> lock(is_running_mutex_);
    if (!is_running_.exchange(false) || !update_task_future_.IsValid()) {
      return;
    }
  }
  is_running_cv_.NotifyAll();
  update_task_future_.Get();
}

void UpdatingComponentBase::DoPeriodicUpdate() {
  const auto steady_now = std::chrono::steady_clock::now();
  auto update_type = UpdateType::kFull;
  if (last_full_update_ + full_update_interval_ > steady_now) {
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
